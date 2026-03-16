struct VertexInput{
    float3 position;
    float3 normal;
    float3 tangent;
    float3 biTangent;
    float2 texCoord;
}

struct VertexOut{
    float4 position : SV_Position;
}
struct FragmentInput{
    float3 position;
    float3 normal;
    float3 tangent;
    float3 bitangent;
    float2 texcoord;
}
struct VertexOutput{
    VertexOut std;
    FragmentInput fragInput : FragmentInput;
}


struct PushConstants{
    float4x4 transform;
    uint32_t colorMapIndex;
    uint32_t normalMapIndex;
}

[[vk_push_constant]]
PushConstants pushConstants;

struct Camera{
    float4x4 viewProjectionMatrix;
    float4x4 inverseProjection;
    float4x4 viewMatrix;
    float4 cameraPos;
}

struct FrameConstantData{
    Camera camera;
}

struct Light{
    float3 color;
    float intensity;
    float3 position;
}
struct LightData{
    uint32_t numberOfLights;
    Light lights[1024];
}

struct Cluster{
    uint32_t numberOfLights;
    uint32_t firstLight;
}

static const uint32_t maxActiveLights=32*12*8*8;
struct ActiveLightsList {
        Atomic<uint32_t> numberOfLights;
        uint32_t activeLightsIndices[maxActiveLights];
};



[[vk::binding(1,1)]]
ConstantBuffer<LightData> lightData;
[[vk::binding(2,1)]]
StructuredBuffer<Cluster[32*12*8]> clusters;
[[vk::binding(3,1)]]
StructuredBuffer<ActiveLightsList> activeLights;

[[vk::binding(0,1)]]
ConstantBuffer<FrameConstantData> frameConstants;
[[vk::binding(0,2)]]
Texture2D textures[];
[[vk::binding(1,2)]]
SamplerState sampler;

static float zNear=0.01;
static float zFar =500;
static float zSlices=32;
static float xSlices=12;
static float ySlices=8;

[shader("vertex")]
VertexOutput vertexShader(VertexInput input ){
    VertexOutput out;
    float4 pos =  mul(frameConstants.camera.viewProjectionMatrix,mul(pushConstants.transform, float4(input.position,1))) ;
    out.std.position=pos;

    out.fragInput.position=mul(pushConstants.transform,float4(input.position,1)).xyz;
    out.fragInput.normal=normalize(mul(float3x3(pushConstants.transform),input.normal));
    out.fragInput.tangent=normalize(mul(float3x3(pushConstants.transform),input.tangent));
    out.fragInput.bitangent=normalize(mul(float3x3(pushConstants.transform),input.biTangent));
    out.fragInput.texcoord=input.texCoord;

    return out;
}

[shader("fragment")]
float4 fragmentShader(FragmentInput input : FragmentInput,float4 pos :SV_Position){
    int3 fragmentClusterCoord = calculateSliceIndex(pos);
    int clusterIndex= 32*8*fragmentClusterCoord.x +32*fragmentClusterCoord.y + fragmentClusterCoord.z;

    float4 baseColor;
    float3 normalValue;
    if(pushConstants.colorMapIndex==-1){
        baseColor={1};
    }else{
        baseColor=textures[pushConstants.colorMapIndex].Sample(sampler,input.texcoord);
    }

    if(pushConstants.normalMapIndex==-1){
        normalValue=input.normal;
    }
    else{
        float3 normalMapValue=textures[pushConstants.normalMapIndex].Sample(sampler,input.texcoord).xyz;
        normalMapValue=normalMapValue*2-1;
        
        normalValue=normalize(
            normalMapValue.x * input.tangent + 
            normalMapValue.z * input.normal + 
            normalMapValue.y * input.bitangent
        );
    }

    float ambientLight = 0.0;
    float3 allLightsLevel={ambientLight};
    int lightsInCluster = clusters[0][clusterIndex].numberOfLights;
    uint32_t firstLight =clusters[0][clusterIndex].firstLight;
    float3 camDirection = normalize(frameConstants.camera.cameraPos.xyz-input.position);

    for(int i=0;i<lightsInCluster;i++){
        uint32_t currentLightIndex = activeLights[0].activeLightsIndices[firstLight+i];
        float3 lightPosition = lightData.lights[currentLightIndex].position;
        float lightIntensity = lightData.lights[currentLightIndex].intensity;
        float3 lightColor = lightData.lights[currentLightIndex].color;
        float3 lightLevel=0;

        float lightStrenght = 1-clamp(distance(lightPosition,input.position)/lightIntensity,0,1);
        

        //diffuse light
        float3 lightDirrection = normalize(lightPosition-input.position);
        lightLevel+=max(dot(normalValue,lightDirrection),0)*lightStrenght;

        //specular light
        float specularConstant=4;
        float3 reflectedRay = -normalize(reflect(lightDirrection,normalValue));
        float3 cameraDirection = normalize(frameConstants.camera.cameraPos.xyz-input.position);
        float specular=max(0,dot(reflectedRay,camDirection))*lightStrenght;
        lightLevel+=pow(specular,specularConstant);
        
        

        allLightsLevel+=lightColor*lightLevel;
    }
    
    
    return {allLightsLevel*baseColor.xyz,1};
}

int3 calculateSliceIndex(float4 fragmentPos){
    int x = int(fragmentPos.x/90);
    int y = 7 - int(fragmentPos.y/90);
    float  zCoord=fragmentPos.z/fragmentPos.w;
    
    int z =int(
        zSlices*((log2(zCoord)-log2(zNear))/(log2(zFar)-log2(zNear)))
    );
    return {x,y,z};
}

