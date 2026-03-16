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



static float zNear=0.01;
static float zFar =500;
static float zSlices=32;
static float xSlices=12;
static float ySlices = 8;
static float tileSize=90;
struct Camera{
    float4x4 viewProjectionMatrix;
    float4x4 inverseProjection;
    float4x4 viewMatrix;
    float4 cameraPos;
}
struct FrameConstantData{
    Camera camera;
}


[[vk::binding(0,1)]]
ConstantBuffer<FrameConstantData> frameConstants;
[[vk::binding(1,1)]]
ConstantBuffer<LightData> lightData;
[[vk::binding(2,1)]]
RWStructuredBuffer<Cluster[32*8*12]> clusters;
[[vk::binding(3,1)]]
RWStructuredBuffer<ActiveLightsList> activeLights;


static float3 minAABB;
static float3 maxAABB;
[shader("compute")]
[numthreads(4,8,32)]
void main(int3 groupID :SV_GroupID,int3 groupThreadID : SV_GroupThreadID){

    int clusterIndex = groupThreadID.x*32*8+groupThreadID.y*32+groupThreadID.z+groupID.x*32*8*4;

    calculateAABB(groupThreadID,groupID);    

    uint32_t numberOfLights = lightData.numberOfLights;
    const uint32_t maxLightsInCluster=128;
    uint32_t lightsInClusterNum=0;
    uint32_t lightsInCluster[maxLightsInCluster];

    for(int i =0;i<lightData.numberOfLights;i++){
        

        if(cullLights(i)){
            lightsInCluster[lightsInClusterNum]=i;
            lightsInClusterNum+=1;
        }

    }

    GroupMemoryBarrier();
    uint lightStart = activeLights[0].numberOfLights.add(lightsInClusterNum);
    clusters[0][clusterIndex].firstLight=lightStart;
    clusters[0][clusterIndex].numberOfLights=lightsInClusterNum;
    for(int i =0;i<lightsInClusterNum;i++){
        activeLights[0].activeLightsIndices[lightStart+i]=lightsInCluster[i];
    }
}

float lightDistance(float3 position,float intensity){
    float squaredDistance = 0;

    if(position.x<minAABB.x){
        squaredDistance+=pow(minAABB.x-position.x,2);
    }
    if(position.x>maxAABB.x){
        squaredDistance+=pow(position.x-maxAABB.x,2);
    }
    if(position.y<minAABB.y){
        squaredDistance+=pow(minAABB.y-position.y,2);
    }
    if(position.y>maxAABB.y){
        squaredDistance+=pow(position.y-maxAABB.y,2);
    }

    if(position.z<minAABB.z){
        squaredDistance+=pow(minAABB.z-position.z,2);
    }
    if(position.z>maxAABB.z){
        squaredDistance+=pow(position.z-maxAABB.z,2);
    }
    return squaredDistance;
}

bool cullLights(int lightIndex){
    float3 position = mul(frameConstants.camera.viewMatrix,float4(lightData.lights[lightIndex].position,1)).xyz;    
    float intensity = lightData.lights[lightIndex].intensity;
    float squaredDistance = lightDistance(position,intensity);
    return squaredDistance < intensity*intensity;
}

void calculateAABB(int3 groupThreadID,int3 groupID){
    float clusterZnear= zNear*pow(zFar/zNear,float(groupThreadID.z)/zSlices);
    float clusterZfar= zNear*pow(zFar/zNear,float(groupThreadID.z+1)/zSlices);

    float2 maxPointXY={ (float(groupID.x*4+groupThreadID.x+1))/xSlices,(float(groupThreadID.y+1))/ySlices};
    float2 minPointXY={ (float(groupID.x*4+groupThreadID.x))/xSlices,(float(groupThreadID.y))/ySlices};

    maxPointXY=maxPointXY*2-1;//[0,1] -> [-1,1]
    minPointXY=minPointXY*2-1;


    float4 maxPoint={maxPointXY,-1,1};
    float4 minPoint={minPointXY,-1,1};

    float4 maxPointView = mul(frameConstants.camera.inverseProjection,maxPoint);
    float4 minPointView = mul(frameConstants.camera.inverseProjection,minPoint);
    maxPointView/=maxPointView.w;
    minPointView/=minPointView.w;

    float3 maxIntersectNear = planeIntersectPoint(maxPointView.xyz,clusterZnear);
    float3 maxIntersectFar = planeIntersectPoint(maxPointView.xyz,clusterZfar);
    float3 minIntersectNear = planeIntersectPoint(minPointView.xyz,clusterZnear);
    float3 minIntersectFar = planeIntersectPoint(minPointView.xyz,clusterZfar);

    minAABB=min(min(maxIntersectNear,maxIntersectFar),min(minIntersectNear,minIntersectFar));
    maxAABB=max(max(maxIntersectNear,maxIntersectFar),max(minIntersectNear,minIntersectFar));
    
}

float3 planeIntersectPoint(float3 point,float zPlane){
    float k = zPlane/point.z;

    return point*k;
}