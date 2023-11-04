void main(void)
{
	// TODO: what should be set in the vertex shaders?
	//~ gl_TexCoord[0] = gl_MultiTexCoord0;
	//~ gl_Position = gl_Vertex;
	//~ gl_FrontColor = gl_BackColor = gl_Color;
	gl_Position = inVertexPosition;
}
