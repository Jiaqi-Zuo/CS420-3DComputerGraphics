#version 150

in vec3 position;
in vec4 color;
out vec4 col;

uniform int mode;

in vec3 pLeft;
in vec3 pRight;
in vec3 pDown;
in vec3 pUp;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  if(mode == 0)
  {
  	gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
  	col = color;
  }
  else if(mode == 1)
  {
	// v.y is the height
  	float smoothenedHeight = (pLeft.y + pRight.y + pDown.y + pUp.y) / 4.0f;
	float eps = 0.0000001f;
	float inputHeight = position.y;
	vec4 outputColor;
  	outputColor = max(color, vec4(eps)) / max(inputHeight, eps) * smoothenedHeight;
	// alpha set to 1.0
  	outputColor.a = 1.0f;
	vec3 pCenter = (pLeft + pRight + pDown + pUp) / 4.0f;
	// repalce pCenter with the average of four neighbours
	gl_Position = projectionMatrix * modelViewMatrix * vec4(pCenter, 1.0f);
  	col = outputColor;
  }

}
