//--------------------------------------------------------------
//  Version: 1.0
//  MadeBy: WinCat
//  Site: tom-crowley.co.uk
//
//  Info:  buffers baby.
//--------------------------------------------------------------


cbuffer PerFrameConstants  : register(cb0)
{
  float4x4    worldToScreenMatrix;
  float4x4    worldToCameraMatrix; // view
  float4x4 	 cameraToWorldMatrix; // inView
  float4x4    worldMatrix;
  float4x4    viewMatrix;
  float4x4    projectionMatrix;
  float4x4    worldViewProjectionMatrix;
  float4     screensize;
  float         time;
  float         frameTime;
}; 
