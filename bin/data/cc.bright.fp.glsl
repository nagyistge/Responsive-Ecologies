varying vec2 texcoord;
uniform sampler2DRect image;

uniform float brightness;

void main (void)
{
	vec4 texColor  	= texture2DRect(image, texcoord).rgba;
	texColor			*= brightness;
	gl_FragColor   	= vec4 (texColor);
}