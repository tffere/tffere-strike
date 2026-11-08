// Tffere Strike: a native C++17 game. All scene/weapon geometry is original.
#include "gameplay.hpp"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "lighting.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

using strike::Mode;
using strike::WeaponId;
static const Color INK{16,23,29,255}, SAND{213,174,113,255}, PALE{229,231,224,255};
static float clamp01(float x){return std::clamp(x,0.f,1.f);}
static Vector3 V(strike::Vec3 p){return {p.x,p.y,p.z};}
struct Prop{Vector3 pos,scale;Color tint;int material=-1;float angle=0;int shape=0;};
struct Decal{Texture2D image;Vector3 pos;float width,height;};
struct Tracer{Vector3 start,end;float life;};

struct Graphics{
    Model cube{},ball{},tube{};
    Shader lit{};
    RenderTexture2D shadow{};
    Camera3D light{{-32,48,25},{0,0,0},{0,1,0},64,CAMERA_ORTHOGRAPHIC};
    std::array<Texture2D,4> textures{};
    std::vector<Prop> props;
    std::vector<Decal> signs;
    Font font{};
    int locLight,locEye,locFlash,locPower,locShadow,locVM,locTex;
    bool shadowPass=false,shadows=true;
    std::string assetDir;
    Graphics(const std::string& assets):assetDir(assets){
        cube=LoadModelFromMesh(GenMeshCube(1,1,1));
        ball=LoadModelFromMesh(GenMeshSphere(1,16,24));
        tube=LoadModelFromMesh(GenMeshCylinder(1,1,16));
        lit=LoadShaderFromMemory(LIT_VERTEX,LIT_FRAGMENT);
        if(!IsShaderValid(lit)||lit.id==rlGetShaderIdDefault())throw std::runtime_error("Lighting shader failed to compile");
        locLight=GetShaderLocation(lit,"lightVP");locEye=GetShaderLocation(lit,"eye");
        locFlash=GetShaderLocation(lit,"flashPosition");locPower=GetShaderLocation(lit,"flashPower");
        locShadow=GetShaderLocation(lit,"useShadow");locVM=GetShaderLocation(lit,"viewModel");locTex=GetShaderLocation(lit,"textured");
        font=LoadFontEx((assetDir+"/interface.ttf").c_str(),64,nullptr,0);
        SetTextureFilter(font.texture,TEXTURE_FILTER_BILINEAR);
        Image atlas=LoadImage((assetDir+"/materials.png").c_str());
        if(!IsImageValid(atlas))throw std::runtime_error("Missing assets/materials.png; extract the complete ZIP before running");
        for(int i=0;i<4;i++){
            Image part=ImageFromImage(atlas,{float(i%2*atlas.width/2),float(i/2*atlas.height/2),float(atlas.width/2),float(atlas.height/2)});
            textures[i]=LoadTextureFromImage(part);GenTextureMipmaps(&textures[i]);
            SetTextureFilter(textures[i],TEXTURE_FILTER_ANISOTROPIC_8X);SetTextureWrap(textures[i],TEXTURE_WRAP_REPEAT);
            UnloadImage(part);
        }
        UnloadImage(atlas);
        shadow.id=rlLoadFramebuffer();shadow.texture.width=shadow.texture.height=2048;
        rlEnableFramebuffer(shadow.id);
        shadow.depth.id=rlLoadTextureDepth(2048,2048,false);
        shadow.depth.width=shadow.depth.height=2048;
        shadow.depth.format=19;shadow.depth.mipmaps=1;
        rlFramebufferAttach(shadow.id,shadow.depth.id,RL_ATTACHMENT_DEPTH,RL_ATTACHMENT_TEXTURE2D,0);
        rlDisableFramebuffer();
    }
    ~Graphics(){
        for(auto t:textures)UnloadTexture(t);
        for(auto d:signs)UnloadTexture(d.image);
        UnloadFont(font);UnloadModel(cube);UnloadModel(ball);UnloadModel(tube);
        rlUnloadFramebuffer(shadow.id);UnloadShader(lit);
    }
    void uniforms(Vector3 eye,Vector3 flash,float power,bool vm=false){
        int on=shadows?1:0,view=vm?1:0;
        SetShaderValue(lit,locEye,&eye,SHADER_UNIFORM_VEC3);
        SetShaderValue(lit,locFlash,&flash,SHADER_UNIFORM_VEC3);
        SetShaderValue(lit,locPower,&power,SHADER_UNIFORM_FLOAT);
        SetShaderValue(lit,locShadow,&on,SHADER_UNIFORM_INT);
        SetShaderValue(lit,locVM,&view,SHADER_UNIFORM_INT);
        int slot=10;SetShaderValue(lit,GetShaderLocation(lit,"shadowMap"),&slot,SHADER_UNIFORM_INT);
        rlActiveTextureSlot(10);rlEnableTexture(shadow.depth.id);rlActiveTextureSlot(0);
    }
    void mesh(Model& model,Vector3 pos,Vector3 size,Color color,int texture=-1,Vector3 axis={0,1,0},float angle=0){
        model.materials[0].shader=shadowPass?Shader{rlGetShaderIdDefault(),rlGetShaderLocsDefault()}:lit;
        // Shader values are applied before each mesh; DrawModel handles transforms.
        int textured=texture>=0?1:0;
        if(!shadowPass)SetShaderValue(lit,locTex,&textured,SHADER_UNIFORM_INT);
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=texture>=0?textures[texture]:Texture2D{rlGetTextureIdDefault(),1,1,1,7};
        DrawModelEx(model,pos,axis,angle,size,color);
    }
    void box(Vector3 p,Vector3 s,Color c,int texture=-1,float angle=0){mesh(cube,p,s,c,texture,{0,0,1},angle);}
    void sphere(Vector3 p,Vector3 s,Color c){mesh(ball,p,s,c);}
    void link(Vector3 from,Vector3 to,float radius,Color c){
        Vector3 delta=Vector3Subtract(to,from),direction=Vector3Normalize(delta);
        Vector3 axis=Vector3CrossProduct({0,1,0},direction);
        if(Vector3Length(axis)<.001f)axis={1,0,0};
        float angle=Vector3Angle({0,1,0},direction)*RAD2DEG;
        mesh(tube,from,{radius,Vector3Length(delta),radius},c,-1,axis,angle);
    }
    void add(float x,float y,float z,float w,float h,float d,Color c,int tex=-1,float angle=0){props.push_back({{x,y,z},{w,h,d},c,tex,angle,0});}
    void pipe(Vector3 a,Vector3 b,float radius,Color c){
        // Store endpoints for persistent tubular details.
        props.push_back({a,b,c,-1,radius,1});
    }
    void label(std::string text,std::string sub,Vector3 pos,float width,float height,Color bg={184,163,137,255}){
        Image im=GenImageColor(1024,384,bg);
        ImageDrawTextEx(&im,font,text.c_str(),{40,48},90,2,{49,65,67,255});
        ImageDrawTextEx(&im,font,sub.c_str(),{42,208},37,2,{156,81,44,255});
        Texture2D t=LoadTextureFromImage(im);UnloadImage(im);SetTextureFilter(t,TEXTURE_FILTER_BILINEAR);
        signs.push_back({t,pos,width,height});
    }
    void build(const strike::Game& game){
        add(0,-.14f,0,160,.25f,160,WHITE,1);
        for(const auto& o:game.obstacles){
            using K=strike::ObstacleKind;
            if(o.kind==K::Wall||o.kind==K::Building){
                add(o.x,o.h/2,o.z,o.w,o.h,o.d,{235,219,196,255},0);
                add(o.x,o.h+.10f,o.z,o.w+.35f,.22f,o.d+.35f,{70,83,87,255});
                add(o.x,.22f,o.z,o.w+.08f,.44f,o.d+.08f,{122,119,108,255},0);
            }else if(o.kind==K::Container){
                Color base=o.x<0?Color{184,98,50,255}:Color{112,158,151,255};
                add(o.x,o.h/2,o.z,o.w,o.h,o.d,base,3);
                for(float z=o.z-o.d/2+.18f;z<o.z+o.d/2;z+=.38f){
                    add(o.x-o.w/2-.025f,o.h/2,z,.055f,o.h-.12f,.07f,base);
                    add(o.x+o.w/2+.025f,o.h/2,z,.055f,o.h-.12f,.07f,base);
                }
                for(float y:{.10f,o.h-.08f})add(o.x,y,o.z,o.w+.10f,.12f,o.d+.10f,{65,76,73,255});
                for(float dx:{-o.w/2+.15f,o.w/2-.15f}){
                    add(o.x+dx,o.h/2,o.z+o.d/2+.035f,.12f,o.h,.12f,{63,72,70,255});
                    pipe({o.x+dx*.65f,.23f,o.z+o.d/2+.12f},{o.x+dx*.65f,o.h-.2f,o.z+o.d/2+.12f},.045f,{166,172,162,255});
                    for(float yy:{.6f,2.2f})add(o.x+dx*.65f,yy,o.z+o.d/2+.16f,.35f,.06f,.06f,{180,183,167,255});
                }
                label(o.x<0?"C - 417":"T - 228","DEPOT FREIGHT / 07",{o.x,o.h*.68f,o.z+o.d/2+.10f},o.w*.68f,.65f,base);
            }else{
                add(o.x,o.h/2,o.z,o.w,o.h,o.d,{207,174,126,255},2);
                float crateH=std::min(o.h,2.2f);
                for(float zz:{o.z-o.d/2-.035f,o.z+o.d/2+.035f}){
                    for(float yy:{.14f,crateH-.14f})add(o.x,yy,zz,o.w,.17f,.10f,{146,103,64,255},2);
                    for(float xx:{o.x-o.w/2+.1f,o.x+o.w/2-.1f})add(xx,crateH/2,zz,.18f,crateH,.10f,{154,113,75,255},2);
                    add(o.x,crateH/2,zz,.16f,crateH*1.2f,.10f,{159,118,79,255},2,40);
                }
            }
        }
        // Factory façades: doors, lintels, conduit, lamps and recessed windows.
        for(auto b:std::array<Vector3,2>{{{-16,0,-2.88f},{17,0,-8.88f}}}){
            float x=b.x,z=b.z;
            add(x,1.9f,z,5,3.8f,.12f,{78,86,87,255},3);
            for(int i=0;i<22;i++)add(x,.12f+i*.17f,z+.08f,5,.025f,.03f,{124,130,124,255});
            for(float dx:{-2.62f,2.62f}){
                add(x+dx,2,z+.10f,.23f,4.1f,.3f,{76,83,80,255});
                add(x+dx,.65f,z+.33f,.18f,1.3f,.20f,{223,172,53,255});
                for(int i=0;i<5;i++)add(x+dx,.13f+i*.25f,z+.45f,.19f,.09f,.015f,{35,40,39,255});
            }
            add(x,4,z+.12f,5.6f,.25f,.6f,{65,78,81,255});
            add(x,4.12f,z+.2f,2.5f,.13f,.28f,{234,216,165,255});
            for(int i=-1;i<=1;i++){
                add(x+i*2.8f,6.6f,z,2.1f,1.35f,.20f,{60,73,74,255});
                add(x+i*2.8f,6.6f,z+.12f,1.85f,1.1f,.06f,{92,125,132,255});
                add(x+i*2.8f,6.6f,z+.17f,.06f,1.1f,.03f,{45,60,62,255});
                add(x+i*2.8f,6.6f,z+.17f,1.85f,.05f,.03f,{45,60,62,255});
            }
            pipe({x-4.8f,.1f,z+.2f},{x-4.8f,8.3f,z+.2f},.12f,{122,129,120,255});
            for(float y:{1.f,3.f,5.f,7.f})add(x-4.8f,y,z+.18f,.35f,.10f,.28f,{70,80,75,255});
            add(x,8.8f,z-3,3.1f,1.4f,2.2f,{144,148,131,255},3);
            for(float xx:{x-.85f,x+.85f}){
                props.push_back({{xx,8.8f,z-1.87f},{.53f,.53f,.1f},{52,67,67,255},-1,0,2});
                for(float y:{8.45f,8.65f,8.85f,9.05f})add(xx,y,z-1.75f,1.0f,.035f,.04f,{129,137,124,255});
            }
            label(x<0?"DEPOT / A":"SECTOR / B","AUTHORIZED PERSONNEL ONLY",{x,5.15f,z+.15f},5.5f,1.25f);
        }
        label("LOADING BAY","SECTOR 07 / KEEP CLEAR",{3,3.3f,-21.95f},7,2.0f);
        // Catwalk in the back, beyond the playable line of fire.
        add(3,5.0f,-20.5f,14,.32f,2.2f,{63,78,79,255});
        for(float x=-4;x<=10;x+=1.5f){
            pipe({x,5.15f,-19.4f},{x,6.25f,-19.4f},.045f,{204,165,71,255});
        }
        pipe({-4,6.25f,-19.4f},{10,6.25f,-19.4f},.05f,{204,165,71,255});
        for(float x:{-3.5f,9.5f})add(x,2.5f,-20.5f,.32f,5,1,{62,73,72,255});
        // Ground details reinforce scale without adding collision surprises.
        for(float z=-17;z<21;z+=3.4f){
            add(-20.5f,.008f,z,.12f,.012f,1.65f,{195,163,91,255});
            add(20.5f,.008f,z,.12f,.012f,1.65f,{195,163,91,255});
        }
        for(float x=-21;x<23;x+=4.8f)add(x,.004f,0,.015f,.008f,45,{97,91,80,255});
        for(float z=-21;z<23;z+=4.8f)add(0,.004f,z,45,.008f,.015f,{97,91,80,255});
        for(int i=0;i<8;i++){
            float x=-7.5f+i*.2f;
            add(x,.012f,15,.075f,.018f,1.3f,{80,85,78,255});
        }
        // Distant warehouses, a radio mast and mountain silhouettes.
        std::mt19937 rng(88);std::uniform_real_distribution<float> height(9,21);
        for(int i=0;i<15;i++){
            float h=height(rng);add(-74+i*10,h/2,-48,7,h,11,{108,126,129,255});
        }
        for(int n=0;n<11;n++){
            float y=7+n*1.6f,w=2.2f-n*.10f;
            for(float x:{15-w,15+w})pipe({x,y,-24},{x+.035f,y+1.6f,-24},.045f,{124,93,77,255});
            pipe({15-w,y,-24},{15+w,y+1.6f,-24},.035f,{144,111,89,255});
            pipe({15+w,y,-24},{15-w,y+1.6f,-24},.035f,{144,111,89,255});
        }
        pipe({15,23,-24},{15,28,-24},.06f,{97,110,108,255});
        props.push_back({{15,21,-23.8f},{.75f,1,.25f},{185,191,174,255},-1,0,2});
        add(15,20,-24.8f,1,1.5f,.3f,{183,184,168,255});
    }
    void world(){
        for(auto& p:props){
            if(p.shape==1)link(p.pos,p.scale,p.angle,p.tint);
            else if(p.shape==2)sphere(p.pos,p.scale,p.tint);
            else box(p.pos,p.scale,p.tint,p.material,p.angle);
        }
        if(!shadowPass)for(auto& d:signs){
            // Vertical textured quads use a dedicated unlit decal pass.
            Vector3 right{d.width/2,0,0};
            rlSetTexture(d.image.id);rlBegin(RL_QUADS);rlColor4ub(255,255,255,255);
            rlNormal3f(0,0,1);
            rlTexCoord2f(0,1);rlVertex3f(d.pos.x-right.x,d.pos.y-d.height/2,d.pos.z);
            rlTexCoord2f(1,1);rlVertex3f(d.pos.x+right.x,d.pos.y-d.height/2,d.pos.z);
            rlTexCoord2f(1,0);rlVertex3f(d.pos.x+right.x,d.pos.y+d.height/2,d.pos.z);
            rlTexCoord2f(0,0);rlVertex3f(d.pos.x-right.x,d.pos.y+d.height/2,d.pos.z);
            rlEnd();rlSetTexture(0);
        }
    }
    void bot(const strike::Bot& b,float elapsed){
        if(!b.alive())return;
        // Rounded articulated silhouettes, layered armour, gloves and boots.
        rlPushMatrix();rlTranslatef(b.pos.x,0,b.pos.z);rlRotatef(b.yaw*RAD2DEG,0,1,0);
        Color cloth{56,74,72,255},armor{34,49,48,255},webbing{94,102,78,255};
        float gait=std::sin(elapsed*6+b.phase)*.13f;
        for(float side:{-1.f,1.f}){
            float z=gait*side;
            link({side*.14f,.84f,0},{side*.15f,.46f,z},.105f,cloth);
            link({side*.15f,.46f,z},{side*.15f,.12f,-z},.085f,cloth);
            sphere({side*.15f,.45f,z},{.105f,.13f,.11f},armor);
            box({side*.15f,.08f,-z-.06f},{.19f,.16f,.34f},{28,36,35,255});
        }
        sphere({0,1.16f,0},{.30f,.40f,.20f},cloth);
        box({0,1.18f,-.05f},{.52f,.53f,.32f},armor);
        box({0,.89f,0},{.51f,.08f,.32f},{41,48,40,255});
        for(float x:{-.17f,0.f,.17f})box({x,1.11f,-.24f},{.135f,.21f,.08f},webbing);
        for(float side:{-1.f,1.f}){
            link({side*.31f,1.4f,0},{side*.33f,1.12f,-.2f},.095f,cloth);
            link({side*.33f,1.12f,-.2f},{side*.14f,1.24f,-.48f},.075f,cloth);
            sphere({side*.14f,1.24f,-.48f},{.08f,.08f,.10f},{35,41,38,255});
            sphere({side*.32f,1.39f,0},{.11f,.10f,.14f},armor);
        }
        link({0,1.42f,0},{0,1.57f,0},.09f,{157,127,100,255});
        sphere({0,1.69f,0},{.16f,.19f,.155f},{162,136,108,255});
        sphere({0,1.77f,.015f},{.19f,.15f,.19f},{48,66,64,255});
        box({0,1.71f,-.145f},{.27f,.065f,.07f},{21,35,37,255});
        box({0,1.65f,-.16f},{.16f,.09f,.045f},{73,82,69,255});
        box({.32f,1.38f,-.02f},{.018f,.13f,.13f},{199,145,59,255});
        box({.04f,1.24f,-.49f},{.10f,.13f,.42f},{30,35,35,255});
        link({.04f,1.26f,-.65f},{.04f,1.26f,-.96f},.025f,{24,29,29,255});
        box({.04f,1.1f,-.46f},{.07f,.17f,.13f},{37,42,39,255});
        if(b.shotFlash>0&&!shadowPass)sphere({.04f,1.26f,-1.0f},{.06f,.06f,.12f},{255,208,99,255});
        rlPopMatrix();
    }
};

struct Audio{
    bool ready=false,muted=false;std::array<Sound,7> sounds{};
    Audio(bool enabled){
        if(!enabled)return;
        InitAudioDevice();ready=IsAudioDeviceReady();if(!ready)return;
        for(int id=0;id<7;id++){
            float duration=id==1?.85f:id==2?.7f:.16f;
            int count=int(44100*duration);std::vector<short> samples(count);
            std::mt19937 rng(71+id);std::uniform_real_distribution<float> noise(-1,1);float low=0;
            for(int i=0;i<count;i++){
                float t=i/44100.f;low=low*.86f+noise(rng)*.14f;float value=0;
                if(id<2){float decay=std::exp(-t*(id==1?9:27));value=(noise(rng)*.38f+low*.65f+std::sin(t*430)*.20f)*decay;
                    if(id==1&&t>.18f)value+=noise(rng)*std::exp(-(t-.18f)*12)*.10f;
                }else if(id==2){float click=std::fmod(t,.22f);value=(noise(rng)*.3f+std::sin(t*1400)*.12f)*std::exp(-click*95);}
                else if(id==3)value=std::sin(t*6500)*std::exp(-t*38)*.22f;
                else if(id==4)value=(low+noise(rng)*.12f)*std::exp(-t*40)*.4f;
                else if(id==5)value=std::sin(t*420)*std::exp(-t*35)*.25f;
                else value=noise(rng)*std::exp(-t*100)*.18f;
                samples[i]=short(std::clamp(value,-.95f,.95f)*28000);
            }
            Wave wave{unsigned(count),44100,16,1,samples.data()};sounds[id]=LoadSoundFromWave(wave);SetSoundVolume(sounds[id],id==1?.75f:.5f);
        }
    }
    void play(int id){if(ready&&!muted)PlaySound(sounds[id]);}
    ~Audio(){if(ready){for(auto s:sounds)UnloadSound(s);CloseAudioDevice();}}
};

static void text(Font f,const char* str,float x,float y,float size,Color c,float spacing=1){DrawTextEx(f,str,{x,y},size,spacing,c);}
static void center(Font f,const char* str,float x,float y,float size,Color c){text(f,str,x-MeasureTextEx(f,str,size,1).x/2,y,size,c);}
static void panel(Rectangle r,Color c=Color{15,22,28,220}){DrawRectangleRounded(r,.06f,10,c);DrawRectangleRoundedLinesEx(r,.06f,10,1,{85,102,107,90});}

static void weapon(Graphics& g,bool sniper,float elapsed,float recoil,float reload){
    Color metal{50,59,63,255},dark{25,31,34,255},steel{99,112,116,255},olive{111,112,78,255},glove{49,56,47,255};
    float sway=std::sin(elapsed*1.8f)*.0025f;
    rlPushMatrix();rlTranslatef(.27f+sway,-.26f-reload*.32f,-.48f+recoil*.08f);rlRotatef(-recoil*5+reload*24,1,0,0);rlRotatef(reload*20,0,0,1);
    // Forearms and shaped gloves anchor the weapon naturally in view.
    g.link({.08f,-.28f,.38f},{.02f,-.10f,.09f},.073f,{85,96,71,255});
    g.sphere({.015f,-.08f,.055f},{.065f,.072f,.09f},glove);
    g.link({-.29f,-.24f,.27f},{-.09f,-.065f,-.37f},.064f,{85,96,71,255});
    g.sphere({-.07f,-.06f,-.35f},{.065f,.06f,.095f},glove);
    if(sniper){
        g.box({0,0,0},{.095f,.10f,.49f},metal);
        g.box({0,-.045f,-.28f},{.115f,.075f,.42f},olive);
        g.box({0,-.035f,.27f},{.12f,.15f,.28f},olive);
        g.box({0,.035f,.20f},{.13f,.035f,.19f},olive);
        g.box({0,-.15f,.04f},{.07f,.17f,.11f},dark, -1,-12);
        g.box({0,-.13f,-.14f},{.075f,.13f,.14f},dark);
        g.link({0,.02f,-.44f},{0,.02f,-1.02f},.023f,dark);
        g.link({0,.02f,-1.02f},{0,.02f,-1.12f},.036f,metal);
        // Scope tube with mount rings, adjustment turrets and cyan glass.
        g.link({0,.147f,.13f},{0,.147f,-.37f},.041f,dark);
        for(float z:{-.22f,.04f}){
            g.box({0,.078f,z},{.065f,.085f,.06f},metal);
            g.link({0,.147f,z-.018f},{0,.147f,z+.018f},.052f,steel);
        }
        g.link({0,.147f,-.33f},{0,.147f,-.46f},.069f,dark);
        g.link({0,.147f,.10f},{0,.147f,.19f},.056f,metal);
        g.link({0,.147f,.19f},{0,.147f,.192f},.048f,{34,100,112,255});
        g.link({0,.147f,-.46f},{0,.147f,-.463f},.061f,{27,78,89,255});
        g.link({0,.18f,-.08f},{0,.215f,-.08f},.025f,steel);
        g.link({.02f,.147f,-.08f},{.065f,.147f,-.08f},.024f,steel);
        g.link({.04f,.01f,.06f},{.105f,-.035f,.11f},.014f,steel);
        g.sphere({.105f,-.035f,.11f},{.025f,.025f,.025f},dark);
        // Folded bipod and vented fore-end.
        for(float side:{-1.f,1.f})g.link({side*.047f,-.02f,-.40f},{side*.06f,-.05f,-.69f},.012f,metal);
        for(int i=0;i<5;i++)g.box({-.059f,-.03f,-.22f-i*.035f},{.006f,.025f,.018f},dark);
    }else{
        g.box({0,0,0},{.105f,.14f,.39f},metal);
        g.box({0,.065f,-.05f},{.07f,.045f,.43f},dark);
        g.box({0,-.03f,-.30f},{.115f,.12f,.30f},metal);
        g.link({0,.01f,-.43f},{0,.01f,-.83f},.020f,dark);
        g.link({0,.01f,-.83f},{0,.01f,-.91f},.030f,steel);
        g.box({0,-.04f,.29f},{.10f,.12f,.24f},dark);
        g.box({0,-.03f,.43f},{.13f,.18f,.045f},dark);
        g.box({0,-.15f,.06f},{.075f,.19f,.105f},dark,-1,-12);
        g.box({0,-.17f,-.12f},{.080f,.25f,.12f},metal,-1,9);
        for(int i=0;i<5;i++)g.box({-.042f,-.10f-i*.035f,-.12f},{.005f,.012f,.09f},steel);
        for(int i=0;i<13;i++)g.box({0,.101f,.12f-i*.040f},{.09f,.010f,.018f},steel);
        for(float side:{-1.f,1.f})for(int i=0;i<6;i++)g.box({side*.059f,.003f,-.20f-i*.035f},{.008f,.052f,.019f},dark);
        g.box({0,.135f,-.67f},{.017f,.14f,.025f},dark);
        g.box({-.031f,.12f,.10f},{.018f,.08f,.04f},dark);
        g.box({.031f,.12f,.10f},{.018f,.08f,.04f},dark);
        g.box({0,.16f,.10f},{.078f,.015f,.04f},dark);
        g.link({-.026f,.02f,-.67f},{0,.12f,-.67f},.009f,dark);
        g.link({.026f,.02f,-.67f},{0,.12f,-.67f},.009f,dark);
        g.box({.057f,.025f,0},{.012f,.045f,.12f},{16,24,24,255});
        g.link({.045f,.015f,.08f},{.082f,.035f,.08f},.009f,steel);
        g.link({-.047f,-.08f,.08f},{-.047f,-.19f,.01f},.009f,steel);
    }
    if(recoil>.45f){
        Vector3 p{0,.014f,sniper?-1.14f:-.93f};
        g.sphere(p,{.045f,.045f,.10f},{255,211,119,255});
        g.link(p,{0,.014f,p.z-.17f},.015f,{255,241,188,255});
    }
    rlPopMatrix();
}

int main(int argc,char** argv){
    bool smoke=false,capture=false;
    for(int i=1;i<argc;i++){if(!std::strcmp(argv[i],"--smoke-test"))smoke=true;if(!std::strcmp(argv[i],"--capture"))capture=true;}
    std::filesystem::path base=std::filesystem::absolute(argv[0]).parent_path();
    SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_WINDOW_RESIZABLE|FLAG_MSAA_4X_HINT|FLAG_VSYNC_HINT);
    InitWindow(1280,720,"Tffere Strike | Native C++");base=GetApplicationDirectory();SetWindowMinSize(960,540);SetExitKey(KEY_NULL);SetTargetFPS(smoke?0:120);
    int exitCode=0;
    try{
        std::filesystem::path assets=base/"assets";
        if(!std::filesystem::exists(assets/"materials.png"))assets=std::filesystem::path(GetWorkingDirectory())/"native/assets";
        Graphics gfx(assets.string());strike::Game game;gfx.build(game);Audio audio(!smoke);
        float recoil=0,scope=0,walk=0,stepTimer=0,switchAnim=0,hitFade=0;
        WeaponId previous=WeaponId::AR7;int frames=0;bool debug=false,blockFire=false;std::vector<Tracer> tracers;
        if(smoke||capture)game.start();
        while(!WindowShouldClose()){
            float dt=std::min(.05f,GetFrameTime());if(smoke)dt=1.f/60;
            float sw=float(GetScreenWidth()),sh=float(GetScreenHeight()),ui=std::min(sw/1280.f,sh/720.f);
            Rectangle playButton{sw/2-200*ui,sh/2+52*ui,400*ui,56*ui};
            if(IsKeyPressed(KEY_F11))ToggleBorderlessWindowed();
            if(IsKeyPressed(KEY_F3))debug=!debug;
            if(IsKeyPressed(KEY_F4))gfx.shadows=!gfx.shadows;
            if(IsKeyPressed(KEY_M))audio.muted=!audio.muted;
            strike::Input input;
            if(game.mode==Mode::Playing){
                input.forward=IsKeyDown(KEY_W);input.backward=IsKeyDown(KEY_S);input.left=IsKeyDown(KEY_A);input.right=IsKeyDown(KEY_D);
                input.sprint=IsKeyDown(KEY_LEFT_SHIFT);input.jump=IsKeyPressed(KEY_SPACE);input.fire=IsMouseButtonDown(MOUSE_BUTTON_LEFT)&&!blockFire;
                if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))blockFire=false;
                input.reload=IsKeyPressed(KEY_R);input.toggleScope=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
                input.selectAR7=IsKeyPressed(KEY_ONE);input.selectAW9=IsKeyPressed(KEY_TWO);
                input.pause=IsKeyPressed(KEY_ESCAPE)||(!IsWindowFocused()&&!smoke);
                Vector2 mouse=GetMouseDelta();float sensitivity=game.scoped?.00075f:.0022f;
                input.lookX=mouse.x*sensitivity;input.lookY=-mouse.y*sensitivity;
            }else if(IsKeyPressed(KEY_ENTER)||(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(GetMousePosition(),playButton)))input.start=true;
            if(game.mode==Mode::Paused&&IsKeyPressed(KEY_R))input.restart=true;
            if(game.mode!=Mode::Playing&&IsKeyPressed(KEY_ESCAPE)){if(game.mode==Mode::Paused)input.start=true;else break;}
            if(capture)input={};
            if(smoke){input={};if(frames<35)input.forward=true;if(frames==40)input.selectAW9=true;if(frames==48)input.toggleScope=true;if(frames==80)input.fire=true;}
            Mode oldMode=game.mode;game.step(dt,input);
            if(game.mode==Mode::Playing&&oldMode!=Mode::Playing&&!smoke){DisableCursor();blockFire=true;}
            if(game.mode!=Mode::Playing&&oldMode==Mode::Playing)EnableCursor();
            if(game.selectedWeapon!=previous){switchAnim=1;previous=game.selectedWeapon;}
            if(game.mode==Mode::Playing){
                bool moving=input.forward||input.backward||input.left||input.right;
                walk+=dt*(moving?(input.sprint?12:8):1.2f);
                if(moving){stepTimer-=dt;if(stepTimer<=0){audio.play(4);stepTimer=input.sprint?.27f:.39f;}}
                scope+=(float(game.scoped)-scope)*std::min(1.f,dt*14);
                recoil=std::max(0.f,recoil-dt*10);switchAnim=std::max(0.f,switchAnim-dt*4);
            }
            for(const auto& e:game.events){
                using E=strike::EventType;
                if(e.type==E::PlayerShot){audio.play(e.weapon==WeaponId::AW9?1:0);recoil=1;tracers.push_back({V(game.lastShot.origin),V(game.lastShot.point),.055f});}
                if(e.type==E::ReloadStarted)audio.play(2);
                if(e.type==E::BotHit){audio.play(3);hitFade=1;}
                if(e.type==E::BotKilled)audio.play(5);
                if(e.type==E::EmptyClick)audio.play(6);
                if(e.type==E::BotShot){audio.play(6);tracers.push_back({{e.position.x,1.3f,e.position.z},{game.player.pos.x,game.player.y,game.player.pos.z},.04f});}
            }
            hitFade=std::max(0.f,hitFade-dt*5);
            for(auto& t:tracers)t.life-=dt;
            tracers.erase(std::remove_if(tracers.begin(),tracers.end(),[](auto t){return t.life<=0;}),tracers.end());
            float bob=game.mode==Mode::Playing&&(input.forward||input.backward||input.left||input.right)?std::sin(walk)*.025f*(1-scope):0;
            Vector3 eye{game.player.pos.x,game.player.y+bob,game.player.pos.z};
            Vector3 direction=V(strike::viewDirection(game.player.yaw,game.player.pitch));
            Camera3D camera{eye,Vector3Add(eye,direction),{0,1,0},76-scope*56,CAMERA_PERSPECTIVE};
            if(game.mode==Mode::Ready){camera.position={.3f,2.1f,18};camera.target={0,2,-8};}
            // Shadow geometry is the same scene and bots used for the colour pass.
            if(gfx.shadows){
                gfx.shadowPass=true;BeginTextureMode(gfx.shadow);ClearBackground(WHITE);BeginMode3D(gfx.light);
                Matrix lightView=rlGetMatrixModelview(),lightProjection=rlGetMatrixProjection();
                SetShaderValueMatrix(gfx.lit,gfx.locLight,MatrixMultiply(lightView,lightProjection));
                gfx.world();for(const auto& b:game.bots)gfx.bot(b,game.elapsed);
                EndMode3D();EndTextureMode();gfx.shadowPass=false;
            }
            BeginDrawing();ClearBackground({160,183,194,255});
            DrawRectangleGradientV(0,0,int(sw),int(sh),{99,135,159,255},{205,205,185,255});
            BeginMode3D(camera);gfx.uniforms(camera.position,eye,game.muzzleFlash>0?1.2f:0);
            gfx.world();for(const auto& b:game.bots)gfx.bot(b,game.elapsed);
            for(auto t:tracers)DrawLine3D(t.start,t.end,{255,218,137,160});
            EndMode3D();
            // A dedicated view camera keeps the hands and weapons properly framed.
            if(game.mode!=Mode::Ready&&scope<.98f){
                Camera3D view{{0,0,0},{0,0,-1},{0,1,0},62,CAMERA_PERSPECTIVE};
                BeginMode3D(view);gfx.uniforms({0,0,0},{0,0,-.5f},game.muzzleFlash>0?.5f:0,true);
                float reload=game.weapon().reloadRemaining>0?std::sin(game.weapon().reloadRemaining/game.weapon().spec.reloadSeconds*PI):0;
                weapon(gfx,game.selectedWeapon==WeaponId::AW9,walk,recoil,std::max(reload,switchAnim));EndMode3D();
            }
            if(scope>.01f){
                float radius=std::min(sw,sh)*.47f, cx=sw/2,cy=sh/2;
                // Scissor the surrounding mask into rectangles and circular fans.
                Color black{5,9,12,(unsigned char)(scope*252)};
                DrawRectangle(0,0,int(cx-radius),int(sh),black);DrawRectangle(int(cx+radius),0,int(sw-cx+radius),int(sh),black);
                for(int y=0;y<int(sh);y+=3){float dy=y-cy;float half=std::abs(dy)<radius?std::sqrt(radius*radius-dy*dy):0;
                    DrawRectangle(int(cx-radius),y,int(radius-half)+1,3,black);DrawRectangle(int(cx+half),y,int(radius-half)+2,3,black);}
                DrawCircleLines(int(cx),int(cy),radius,{56,75,78,(unsigned char)(scope*255)});
                DrawLine(int(cx-radius),int(cy),int(cx+radius),int(cy),{7,12,12,(unsigned char)(scope*255)});
                DrawLine(int(cx),int(cy-radius),int(cx),int(cy+radius),{7,12,12,(unsigned char)(scope*255)});
                for(int n=-4;n<=4;n++)if(n){DrawLine(int(cx+n*radius*.12f),int(cy-4),int(cx+n*radius*.12f),int(cy+4),BLACK);DrawLine(int(cx-4),int(cy+n*radius*.12f),int(cx+4),int(cy+n*radius*.12f),BLACK);}
                DrawCircle(int(cx),int(cy),2,{181,75,55,(unsigned char)(scope*255)});
            }
            if(game.mode!=Mode::Ready){
                // HUD uses native vector panels and an embedded TrueType font.
                DrawRectangleGradientV(0,0,int(sw),int(105*ui),{10,17,23,205},{10,17,23,0});
                DrawRectangleGradientV(0,int(sh-120*ui),int(sw),int(120*ui),{10,17,23,0},{10,17,23,220});
                text(gfx.font,"TFFERE",30*ui,24*ui,22*ui,PALE,3*ui);text(gfx.font,"STRIKE",139*ui,24*ui,22*ui,SAND,3*ui);
                text(gfx.font,"NATIVE / TACTICAL TRAINING",31*ui,53*ui,9*ui,{161,178,183,255},2*ui);
                center(gfx.font,"DEPOT / SECTOR 07",sw/2,26*ui,11*ui,SAND);
                center(gfx.font,"ELIMINATE THE PATROL",sw/2,48*ui,15*ui,PALE);
                int seconds=std::max(0,int(game.timeLeft));
                text(gfx.font,TextFormat("%02d:%02d",seconds/60,seconds%60),sw-115*ui,28*ui,29*ui,game.timeLeft<30?Color{231,120,95,255}:PALE);
                text(gfx.font,"HEALTH",30*ui,sh-90*ui,10*ui,{170,186,190,255},2*ui);
                text(gfx.font,TextFormat("%03d",game.player.health),30*ui,sh-72*ui,40*ui,PALE);
                DrawRectangle(32*ui,sh-20*ui,120*ui,3*ui,{66,76,80,255});DrawRectangle(32*ui,sh-20*ui,120*ui*game.player.health/100,3*ui,SAND);
                center(gfx.font,"HOSTILES REMAINING",sw/2,sh-74*ui,10*ui,{174,186,185,255});
                center(gfx.font,TextFormat("%d / 06",game.remaining()),sw/2,sh-54*ui,30*ui,PALE);
                const auto& w=game.weapon();
                text(gfx.font,game.selectedWeapon==WeaponId::AW9?"AW-9 / BOLT ACTION":"AR-7 / ASSAULT RIFLE",sw-260*ui,sh-90*ui,10*ui,SAND);
                text(gfx.font,TextFormat("%02d / %02d",w.ammo,w.reserve),sw-206*ui,sh-69*ui,40*ui,PALE);
                text(gfx.font,"1  AR-7     2  AW-9",sw-241*ui,sh-23*ui,10*ui,{156,173,178,255});
                if(w.reloadRemaining>0){center(gfx.font,"RELOADING",sw/2,sh/2+55*ui,12*ui,SAND);DrawRectangle(sw/2-70*ui,sh/2+77*ui,140*ui*(1-w.reloadRemaining/w.spec.reloadSeconds),3*ui,SAND);}
                if(scope<.5f&&game.mode==Mode::Playing){
                    int x=sw/2,y=sh/2,gap=6+recoil*8;
                    DrawLine(x-gap-6,y,x-gap,y,PALE);DrawLine(x+gap,y,x+gap+6,y,PALE);DrawLine(x,y-gap-6,x,y-gap,PALE);DrawLine(x,y+gap,x,y+gap+6,PALE);
                }
                if(hitFade>0){Color c{233,218,183,(unsigned char)(hitFade*255)};int x=sw/2,y=sh/2;
                    for(int a:{-1,1})for(int b:{-1,1})DrawLine(x+a*8,y+b*8,x+a*15,y+b*15,c);}
                // Compact radar, with real collision geometry and live bot markers.
                float rx=95*ui,ry=141*ui,rr=59*ui;
                DrawCircle(int(rx),int(ry),rr,{16,28,31,190});DrawCircleLines(int(rx),int(ry),rr,{127,151,151,180});
                float k=rr/25;
                for(auto o:game.obstacles)DrawRectangle(rx+(o.x-o.w/2)*k,ry+(o.z-o.d/2)*k,o.w*k,o.d*k,{104,120,114,190});
                for(auto b:game.bots)if(b.alive())DrawCircle(int(rx+b.pos.x*k),int(ry+b.pos.z*k),2.5f*ui,{222,139,109,255});
                Vector2 p{rx+game.player.pos.x*k,ry+game.player.pos.z*k};
                DrawCircleV(p,3*ui,SAND);DrawLineV(p,{p.x-std::sin(game.player.yaw)*9*ui,p.y-std::cos(game.player.yaw)*9*ui},PALE);
                if(game.damageFlash>0)DrawRectangle(0,0,int(sw),int(sh),{150,35,24,(unsigned char)(std::min(.22f,game.damageFlash)*255)});
                if(audio.muted)text(gfx.font,"AUDIO MUTED / M",sw-160*ui,78*ui,10*ui,SAND);
            }
            if(game.mode!=Mode::Playing){
                DrawRectangle(0,0,int(sw),int(sh),{7,14,20,175});
                Rectangle card{sw/2-250*ui,sh/2-285*ui,500*ui,570*ui};panel(card,{14,22,29,245});
                text(gfx.font,"ORIGINAL SINGLE-PLAYER FPS",card.x+38*ui,card.y+35*ui,10*ui,{144,164,173,255},3*ui);
                text(gfx.font,"TFFERE",card.x+35*ui,card.y+78*ui,60*ui,PALE,4*ui);
                text(gfx.font,"STRIKE",card.x+35*ui,card.y+142*ui,60*ui,SAND,4*ui);
                DrawRectangle(card.x+39*ui,card.y+222*ui,54*ui,2*ui,SAND);
                const char* title=game.mode==Mode::Ready?"DEPOT / SECTOR 07":game.mode==Mode::Paused?"MISSION PAUSED":game.mode==Mode::Won?"DEPOT SECURED":"MISSION FAILED";
                text(gfx.font,title,card.x+39*ui,card.y+248*ui,17*ui,PALE,2*ui);
                const char* copy=game.mode==Mode::Ready?"Six patrol units. One industrial courtyard.":game.mode==Mode::Paused?"Your round is paused. Resume when ready.":game.mode==Mode::Won?"All patrol units eliminated. Good work.":"Change lanes, use cover, and try again.";
                text(gfx.font,copy,card.x+39*ui,card.y+283*ui,14*ui,{166,185,192,255});
                bool hover=CheckCollisionPointRec(GetMousePosition(),playButton);
                DrawRectangleRec(playButton,hover?Color{232,195,135,255}:SAND);
                const char* action=game.mode==Mode::Paused?"RESUME MISSION":"DEPLOY INTO DEPOT";
                text(gfx.font,action,playButton.x+24*ui,playButton.y+19*ui,13*ui,INK,2*ui);
                text(gfx.font,"WASD   MOVE       MOUSE   LOOK / FIRE",card.x+39*ui,card.y+425*ui,11*ui,PALE);
                text(gfx.font,"1 / 2  WEAPONS    RIGHT CLICK   SCOPE",card.x+39*ui,card.y+453*ui,11*ui,PALE);
                text(gfx.font,"R  RELOAD    SPACE  JUMP    SHIFT  SPRINT",card.x+39*ui,card.y+481*ui,11*ui,PALE);
                text(gfx.font,"ENTER / START     ESC / PAUSE     F11 / FULLSCREEN",card.x+39*ui,card.y+523*ui,9*ui,{130,152,163,255});
            }
            if(debug)text(gfx.font,TextFormat("%d FPS | SHADOWS %s [F4] | M AUDIO",GetFPS(),gfx.shadows?"ON":"OFF"),15,sh-15,11,WHITE);
            EndDrawing();frames++;
            if((smoke&&frames==110)||(capture&&frames==3)){
                TakeScreenshot((base/"native-gameplay.png").string().c_str());
                if(smoke){
                    if(!IsWindowReady()||!IsShaderValid(gfx.lit)||game.player.pos.z>=18||game.selectedWeapon!=WeaponId::AW9||game.weapon().ammo!=4)exitCode=2;
                    std::printf("NATIVE SMOKE %s: frame rendering, movement, sniper selection, zoom, shot; %d props\n",exitCode?"FAIL":"PASS",int(gfx.props.size()));
                }
                break;
            }
        }
    }catch(const std::exception& e){
        std::fprintf(stderr,"Tffere Strike: %s\n",e.what());exitCode=1;
        if(!smoke&&IsWindowReady()){
            SetWindowTitle("Tffere Strike - Startup problem");EnableCursor();
            while(!WindowShouldClose()&&!IsKeyPressed(KEY_ENTER)&&!IsKeyPressed(KEY_ESCAPE)){
                BeginDrawing();ClearBackground(INK);
                DrawText("TFFERE STRIKE COULD NOT START",38,45,25,SAND);
                DrawText(e.what(),38,105,17,PALE);
                DrawText("Extract the entire ZIP before launching. Keep the assets folder beside the EXE.",38,170,17,PALE);
                DrawText("Press Enter or Escape to close.",38,225,17,SAND);EndDrawing();
            }
        }
    }
    CloseWindow();return exitCode;
}
