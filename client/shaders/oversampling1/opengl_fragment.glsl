uniform sampler2D baseTexture;

uniform vec2 u_base_pixlen;

vec3 srgb_to_ycbcr(vec3 col_srgb)
{
	vec3 col_rgb = pow(col_srgb, vec3(1.0f / 2.2f));
	float y = 0.299f * col_rgb.r + 0.587f * col_rgb.g + 0.114f * col_rgb.b;
	float cb = (-0.168736f * col_rgb.r - 0.331264f * col_rgb.g
		+ 0.5f * col_rgb.b) + 0.5f;
	float cr = (0.5f * col_rgb.r - 0.418688f * col_rgb.g
		- 0.081312f * col_rgb.b) + 0.5f;
	return vec3(y, cb, cr);
	// TODO: ycbcr via matrix mult.
}

void main()
{
	// Convert to YCbCr and calculate L and L2
	// gl_FragCoord is offset by 0.5 (pixel center), which needs to be
	// compensated for when searching for the lower left pixel (x1, y2)
	float x1 = (gl_FragCoord.x - 0.5f) * 2 + 0.5;
	float y1 = (gl_FragCoord.y - 0.5f) * 2 + 0.5;
	vec3 col_ycbcr = vec3(0);
	vec3 col_ycbcr_squareds = vec3(0);
	for (float y = y1; y < y1+2; y += 1) {
		for (float x = x1; x < x1+2; x += 1) {
			vec2 texcoords = vec2(x, y) * u_base_pixlen;
			vec3 sample_col_srgb = texture2D(baseTexture, texcoords).rgb;
			vec3 sample_col_ycbcr = srgb_to_ycbcr(sample_col_srgb);
			col_ycbcr += sample_col_ycbcr;
			// Component-wise multiplication
			col_ycbcr_squareds += sample_col_ycbcr * sample_col_ycbcr;
		}
	}
	col_ycbcr *= 0.25f;
	col_ycbcr_squareds *= 0.25f;

	gl_FragData[0] = vec4(col_ycbcr, 1.0);
	gl_FragData[1] = vec4(col_ycbcr_squareds, 1.0);
}
