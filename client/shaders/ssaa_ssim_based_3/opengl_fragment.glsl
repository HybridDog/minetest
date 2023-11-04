#define input_l texture0
#define input_m texture1
#define input_r texture2

uniform sampler2D input_l;
uniform sampler2D input_m;
uniform sampler2D input_r;
uniform vec2 texelSize0;


vec3 ycbcr_to_linear_srgb(vec3 col_ycbcr)
{
	float y = col_ycbcr.x;
	float cb = col_ycbcr.y;
	float cr = col_ycbcr.z;
	float r = (y + 1.402f * (cr - 0.5f));
	float g = (y - 0.344136f * (cb - 0.5f) - 0.714136f * (cr - 0.5f));
	float b = (y + 1.772f * (cb - 0.5f));
	return vec3(r, g, b);
}

void main()
{
	float x1 = gl_FragCoord.x;
	float y1 = gl_FragCoord.y;
	vec3 col_l = texture2D(input_l, vec2(x1, y1) * texelSize0).rgb;
	vec3 acc_d = vec3(0);
	for (float y = y1; y > y1-2; y -= 1) {
		for (float x = x1; x > x1-2; x -= 1) {
			vec2 texcoords = vec2(x, y) * texelSize0;
			vec3 col_m = texture2D(input_m, texcoords).rgb;
			vec3 col_r = texture2D(input_r, texcoords).rgb;
			acc_d += col_m + col_r * col_l - col_r * col_m;
		}
	}
	acc_d = acc_d * 0.25;

	// TODO: Decide if it looks better if only the Y component is sharpened
	//~ acc_d.yz = col_l.yz;

	// Go back to sRGB colours
	gl_FragColor = vec4(ycbcr_to_linear_srgb(acc_d), 1.0);
}
