//--------------------------------------------------------------
//  Version: 1.0
//  MadeBy: WinCat
//  Site: tom-crowley.co.uk
//
//  Info:  buffers baby.
//--------------------------------------------------------------


cbuffer PerFrameConstants  : register(b0)
{
  float4x4    worldToScreenMatrix;  // worldspace to screenspace (view * proj)
  float4x4    worldToCameraMatrix;  // world to cameraspace (view)
  float4x4 	  cameraToWorldMatrix;  // cameraspace back to worldspace (inView)

  float4      screensize;   // x,y, 0.5/x, 0.5 /y
  float       time;
  float       frameTime;
  float4      cameraPos;
};

cbuffer PerObjectConstants  : register(b1)
{
  float4x4    worldMatrix;
//  float4x4    worldViewProjectionMatrix; // objectspace to screenspace
//  float4x4    objectToWorldMatrix;
};



