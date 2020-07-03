uniform sampler2D baseTexture;

uniform vec2 u_base_pixlen;


#define rendered baseTexture

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec2 p0 = uv - 0.5 * u_base_pixlen;
	vec2 p1 = p0 + vec2(u_base_pixlen.x, 0);
	vec2 p2 = p0 + vec2(0, u_base_pixlen.y);
	vec2 p3 = p0 + vec2(u_base_pixlen.x, u_base_pixlen.y);
	vec3 col = texture2D(rendered, p0).rgb
		+ texture2D(rendered, p1).rgb
		+ texture2D(rendered, p2).rgb
		+ texture2D(rendered, p3).rgb;
	col = col * 0.25;
	gl_FragColor = vec4(col, 1);
	//~ gl_FragColor = 0.1 * vec4(col, 1) + texture2D(rendered, uv).rgba;
}
