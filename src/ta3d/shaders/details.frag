varying vec2 t_coord;
varying vec2 dt_coord;
varying vec3 color;
uniform float coef;
uniform sampler2D tex;
uniform sampler2D details;

void main()
{
	gl_FragColor = vec4( color, coef ) * texture2D( tex, t_coord ) * texture2D( details, dt_coord );
}
