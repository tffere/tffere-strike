#pragma once
// Embedded shaders keep the shipped executable independent of shader paths.
const char* LIT_VERTEX = R"GLSL(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 lightVP;
out vec3 world;
out vec3 normal;
out vec2 uv;
out vec4 tint;
out vec4 lightSpace;
void main(){
 vec4 p=matModel*vec4(vertexPosition,1.0);
 world=p.xyz; normal=normalize(vec3(matNormal*vec4(vertexNormal,0.0)));
 uv=vertexTexCoord;tint=vertexColor;lightSpace=lightVP*p;
 gl_Position=mvp*vec4(vertexPosition,1.0);
})GLSL";
const char* LIT_FRAGMENT = R"GLSL(#version 330
in vec3 world;in vec3 normal;in vec2 uv;in vec4 tint;in vec4 lightSpace;
uniform sampler2D texture0;
uniform sampler2D shadowMap;
uniform vec4 colDiffuse;
uniform vec3 eye;
uniform vec3 flashPosition;
uniform float flashPower;
uniform int useShadow;
uniform int viewModel;
uniform int textured;
uniform float fogDensity;
uniform float textureBlend;
uniform int clothUV;
out vec4 finalColor;
void main(){
 vec3 n=normalize(normal), l=normalize(vec3(-.65,1.0,.45));
 vec2 tc=abs(n.y)>.5?world.xz*.24:(abs(n.x)>.5?world.zy*.24:world.xy*.24);
 vec3 albedo=colDiffuse.rgb*tint.rgb;
 if(textured==1)albedo*=mix(vec3(1.0),texture(texture0,clothUV==1?uv*3.0:tc).rgb,clothUV==1?1.0:textureBlend);
 float ndl=max(dot(n,l),0.0), shade=0.0;
 if(useShadow==1&&viewModel==0){
  vec3 p=lightSpace.xyz/lightSpace.w*.5+.5;
  if(p.z<1.0&&p.x>0.0&&p.x<1.0&&p.y>0.0&&p.y<1.0){
   float bias=max(.0015*(1.0-ndl),.0008);
   vec2 texel=1.0/vec2(textureSize(shadowMap,0));
   for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
    shade+=(p.z-bias>texture(shadowMap,p.xy+vec2(x,y)*texel).r)?1.0:0.0;
   shade/=9.0;
  }
 }
 vec3 ambient=mix(vec3(.29,.31,.34),vec3(.48,.52,.56),n.y*.5+.5);
 if(viewModel==1)albedo*=.96+.04*fract(sin(dot(world*850.0,vec3(12.98,78.23,39.42)))*43758.54);
 vec3 color=albedo*(ambient+vec3(.85,.76,.62)*ndl*(1.0-.72*shade));
 vec3 halfDir=normalize(l+normalize(eye-world));
 float spec=pow(max(dot(n,halfDir),0.0),40.0)*.10*(1.0-shade);
 color+=spec*vec3(1.0,.90,.75);
 float dist=length(world-flashPosition);
 color+=albedo*vec3(1.0,.55,.16)*flashPower/(1.0+dist*dist*4.0);
 if(viewModel==0){
  float fog=1.0-exp(-length(eye-world)*fogDensity);
  color=mix(color,vec3(.58,.65,.69),fog);
 }
 finalColor=vec4(color,1.0);
})GLSL";
