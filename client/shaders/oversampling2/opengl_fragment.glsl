uniform sampler2D baseTexture;
uniform sampler2D normalTexture;

uniform vec2 size_divisor;

#define input_l baseTexture
#define input_l2 normalTexture

float patch_sz_div = 0.25f;

void main()
{
	vec3 acc_m = vec3(0);
	vec3 acc_r_1 = vec3(0);
	vec3 acc_r_2 = vec3(0);
	float x1 = gl_FragCoord.x;
	float y1 = gl_FragCoord.y;
	// Patch size is 2x2
	for (float y = y1; y < y1+2; y += 1) {
		for (float x = x1; x < x1+2; x += 1) {
			vec2 texcoords = vec2(x, y) * size_divisor;
			vec3 col_l = texture2D(input_l, texcoords).rgb;
			vec3 col_l2 = texture2D(input_l2, texcoords).rgb;
			acc_m += col_l;
			acc_r_1 += col_l * col_l;
			acc_r_2 += col_l2;
		}
	}

	vec3 mv = acc_m * patch_sz_div;
	vec3 slv = acc_r_1 * patch_sz_div - mv * mv;
	vec3 shv = acc_r_2 * patch_sz_div - mv * mv;

	vec3 rv;
	for (int i = 0; i < 3; ++i) {
		// big numerical errors happen here, adjust this threshold when needed
		if (slv[i] >= 0.0005f)
			rv[i] = sqrt(shv[i] / slv[i]);
		else
			rv[i] = 0;
	}

	gl_FragData[0] = vec4(mv, 1.0);
	gl_FragData[1] = vec4(rv, 1.0);
}
