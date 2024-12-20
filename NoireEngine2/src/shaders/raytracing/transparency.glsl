struct hitPayload
{
    vec3 hitValue;
    int  depth;
    vec3 rayOrigin;
    int  done;
    vec3 rayDir;
    vec3 attenuation;
    int writeToMask;
};

layout(push_constant) uniform constants 
{ 
    uint rayDepth;
} rayConstants;