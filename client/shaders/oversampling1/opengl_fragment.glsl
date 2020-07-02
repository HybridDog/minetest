uniform sampler2D baseTexture;

#define rendered baseTexture

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(rendered, uv).rgba;
	gl_FragColor = color;
}
