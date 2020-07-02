uniform sampler2D baseTexture;

uniform vec2 resolution;


#define rendered baseTexture

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec2 pixlen = vec2(1 / resolution.x, 1 / resolution.y);
	vec2 p0 = uv - 0.5 * pixlen;
	vec2 p1 = p0 + vec2(pixlen.x, 0);
	vec2 p2 = p0 + vec2(0, pixlen.y);
	vec2 p3 = p0 + vec2(pixlen.x, pixlen.y);
	vec3 col = texture2D(rendered, p0).rgb
		+ texture2D(rendered, p1).rgb
		+ texture2D(rendered, p2).rgb
		+ texture2D(rendered, p3).rgb;
	gl_FragColor = vec4(col, 1);
	//~ gl_FragColor = 0.1 * vec4(col, 1) + texture2D(rendered, uv).rgba;
}
