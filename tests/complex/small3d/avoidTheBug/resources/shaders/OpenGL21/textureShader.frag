#version 120

varying float cosAngIncidence;
varying vec2 textureCoords;
uniform sampler2D textureImage;
uniform vec4 colour;
uniform float lightIntensity;

void main()
{
  if (colour != vec4(0, 0, 0, 0)) {
    gl_FragColor = cosAngIncidence * colour;
}
else {
  if (lightIntensity == -1)
  {
    gl_FragColor = vec4(texture2D(textureImage, textureCoords).rgb, 1.0);
  }
  else
  {
    vec4 textureWtLight = lightIntensity * cosAngIncidence 
			* texture2D(textureImage, textureCoords);
    gl_FragColor = vec4(textureWtLight.rgb, 1.0);
  }
}

}
