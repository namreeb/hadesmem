//--------------------------------------------------------------------------------------
// File: Tutorial02.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
float2 Viewport;

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

VS_OUTPUT VS( float4 Pos : POSITION, float4 Color : COLOR )
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Pos = Pos;
	output.Color = Color;
	
	// Translate our 2D coordinates to 0,0(center) relative coordinates
	output.Pos.x = ( -1.f + (2 * (output.Pos.x / Viewport.x)) );
	output.Pos.y = ( 1.f - (2 * (output.Pos.y / Viewport.y)) );
	
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
    return input.Color;
}

//--------------------------------------------------------------------------------------
technique10 Render
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
