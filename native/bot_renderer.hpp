#pragma once
#include "bot_pose.hpp"
// Original articulated patrol model. Geometry and animation are generated locally.
struct BotRenderer {
    struct Ring {float y,rx,rz,z=0;};
    struct Pose {strike::Vec2 previous{};float yaw=0,phase=0,speed=0,hit=0,death=0,deathDirection=1,deathHip=.94f;int hp=100;bool initialized=false;};
    std::array<Pose,6> poses{};
    Model torso{},limb{},head{},helmet{},boot{},pouch{};
    Texture2D cloth{};
    float previousTime=0;int clothUVLoc=-2;
    static Mesh profile(std::vector<Ring> rings,int slices=20,bool smooth=false){
        Mesh mesh{};int quads=(int(rings.size())-1)*slices;
        mesh.vertexCount=quads*6+slices*6;mesh.triangleCount=mesh.vertexCount/3;
        mesh.vertices=(float*)MemAlloc(mesh.vertexCount*3*sizeof(float));
        mesh.normals=(float*)MemAlloc(mesh.vertexCount*3*sizeof(float));
        mesh.texcoords=(float*)MemAlloc(mesh.vertexCount*2*sizeof(float));int at=0;
        auto point=[&](int ring,int side){auto r=rings[ring];float angle=side*2*PI/slices;return Vector3{std::cos(angle)*r.rx,r.y,std::sin(angle)*r.rz+r.z};};
        auto triangle=[&](Vector3 a,Vector3 b,Vector3 c){
            Vector3 n=Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b,a),Vector3Subtract(c,a)));
            for(auto p:{a,b,c}){int i=at++;mesh.vertices[i*3]=p.x;mesh.vertices[i*3+1]=p.y;mesh.vertices[i*3+2]=p.z;
                Vector3 vn=smooth?Vector3Normalize(Vector3{p.x,(p.y-.13f)*.62f,p.z}):n;
                mesh.normals[i*3]=vn.x;mesh.normals[i*3+1]=vn.y;mesh.normals[i*3+2]=vn.z;
                mesh.texcoords[i*2]=std::atan2(p.z,p.x)/(2*PI)+.5f;mesh.texcoords[i*2+1]=p.y;}
        };
        for(int r=0;r<int(rings.size())-1;r++)for(int s=0;s<slices;s++){
            auto a=point(r,s),b=point(r+1,s),c=point(r+1,s+1),d=point(r,s+1);
            triangle(a,b,c);triangle(a,c,d);
        }
        for(int s=0;s<slices;s++){
            triangle({0,rings.front().y,rings.front().z},point(0,s),point(0,s+1));
            int last=int(rings.size())-1;
            triangle({0,rings.back().y,rings.back().z},point(last,s+1),point(last,s));
        }
        UploadMesh(&mesh,false);return mesh;
    }
    BotRenderer(){
        torso=LoadModelFromMesh(profile({{0,.16f,.105f},{.10f,.18f,.12f},{.37f,.205f,.13f},{.49f,.23f,.125f},{.56f,.18f,.10f}},16));
        limb=LoadModelFromMesh(profile({{0,.95f,.85f},{.14f,1,.96f},{.38f,.96f,1},{.55f,.84f,.85f},{.70f,.86f,.88f},{1,.70f,.70f}},14));
        head=LoadModelFromMesh(profile({{0,.06f,.065f},{.035f,.08f,.08f,-.005f},{.10f,.10f,.091f},{.18f,.10f,.088f},{.235f,.08f,.075f},{.26f,.045f,.045f}},32,true));
        helmet=LoadModelFromMesh(profile({{0,.112f,.115f},{.04f,.118f,.12f},{.09f,.10f,.105f},{.135f,.055f,.06f},{.15f,.015f,.015f}},32,true));
        boot=LoadModelFromMesh(profile({{0,.08f,.12f,-.045f},{.05f,.083f,.15f,-.06f},{.11f,.071f,.12f,-.02f},{.22f,.065f,.068f,.01f}},16));
        pouch=LoadModelFromMesh(profile({{0,.04f,.024f},{.015f,.047f,.028f},{.12f,.046f,.025f},{.14f,.04f,.023f}},12));
        Image image=GenImageColor(128,128,{151,147,116,255});std::mt19937 rng(904);
        Color colors[4]={{211,213,201,255},{230,226,205,255},{176,185,163,255},{219,213,184,255}};
        for(int n=0;n<230;n++)ImageDrawCircle(&image,rng()%128,rng()%128,2+rng()%9,colors[rng()%4]);
        cloth=LoadTextureFromImage(image);UnloadImage(image);GenTextureMipmaps(&cloth);SetTextureFilter(cloth,TEXTURE_FILTER_BILINEAR);
    }
    ~BotRenderer(){UnloadModel(torso);UnloadModel(limb);UnloadModel(head);UnloadModel(helmet);UnloadModel(boot);UnloadModel(pouch);UnloadTexture(cloth);}
    void update(const strike::Game& game,float dt){
        bool reset=game.elapsed<previousTime;previousTime=game.elapsed;
        for(int i=0;i<6;i++){
            auto& p=poses[i];const auto& b=game.bots[i];
            if(!p.initialized||reset){p=Pose{};p.previous=b.pos;p.yaw=b.yaw;p.initialized=true;}
            if(game.mode==Mode::Paused||game.mode==Mode::Ready)continue;
            if(b.hp<=0&&p.hp>0){
                auto forward=strike::viewDirection(b.yaw,0);
                float dx=game.lastShot.origin.x-b.pos.x,dz=game.lastShot.origin.z-b.pos.z;
                p.deathDirection=forward.x*dx+forward.z*dz>=0?1.f:-1.f;
                auto blockedFall=[&](float direction){return strike::circleBlocked(
                    {b.pos.x-forward.x*direction,b.pos.z-forward.z*direction},.3f,game.obstacles);};
                if(blockedFall(p.deathDirection)&&!blockedFall(-p.deathDirection))p.deathDirection=-p.deathDirection;
                p.deathHip=.94f-.55f*b.crouch;
            }
            float travelled=strike::distance(p.previous,b.pos);float speed=dt>0?travelled/dt:0;
            p.previous=b.pos;p.speed+=(speed-p.speed)*std::min(1.f,dt*10);
            p.phase+=travelled/(b.speed>2.8f?1.5f:1.15f)*2*PI;
            float turn=std::remainder(b.yaw-p.yaw,2*PI);p.yaw+=turn*std::min(1.f,dt*8);
            p.hit=std::max(0.f,p.hit-dt*5);
            if(b.hp<p.hp&&b.hp>0)p.hit=1;
            p.hp=b.hp;if(!b.alive())p.death=std::min(1.f,p.death+dt*.85f);
        }
    }
    template<class G> void garment(G& g,Model& model,Vector3 pos,Vector3 size,Color color,Vector3 axis={0,1,0},float angle=0){
        auto saved=g.textures[0];g.textures[0]=cloth;
        if(clothUVLoc==-2)clothUVLoc=GetShaderLocation(g.lit,"clothUV");
        int uvMode=1;if(!g.shadowPass)SetShaderValue(g.lit,clothUVLoc,&uvMode,SHADER_UNIFORM_INT);
        g.mesh(model,pos,size,color,0,axis,angle);
        uvMode=0;if(!g.shadowPass)SetShaderValue(g.lit,clothUVLoc,&uvMode,SHADER_UNIFORM_INT);
        g.textures[0]=saved;
    }
    template<class G> void segment(G& g,Vector3 from,Vector3 to,float width,float depth,Color color){
        Vector3 delta=Vector3Subtract(to,from),dir=Vector3Normalize(delta),axis=Vector3CrossProduct({0,1,0},dir);
        if(Vector3Length(axis)<.001f)axis={1,0,0};
        garment(g,limb,from,{width,Vector3Length(delta),depth},color,axis,Vector3Angle({0,1,0},dir)*RAD2DEG);
    }
    template<class G> void draw(G& g,const strike::Bot& b,int index,float elapsed){
        auto& p=poses[index];if(!p.initialized)return;
        float motion=std::clamp(p.speed/1.3f,0.f,1.f),phase=p.phase;
        float running=std::clamp((p.speed-2.f)/1.8f,0.f,1.f);
        float airborne=std::clamp(b.feet*3.f,0.f,1.f);
        float crouch=b.crouch;
        auto fall=strike::collapsePose(p.death,p.deathHip,p.deathDirection,index);
        float collapse=fall.legFold;
        if(!b.alive()){motion*=1-collapse;running*=1-collapse;}
        const float hipY=b.alive()?.94f-.55f*crouch:fall.hipHeight;
        float bounce=std::abs(std::sin(phase))*(.018f+.025f*running)*motion*(1-airborne);
        float breathe=std::sin(elapsed*2.1f+index)*.004f*(1-collapse);
        float lean=std::sin(elapsed*.65f+index)*.025f*(1-motion);
        rlPushMatrix();rlTranslatef(b.pos.x,b.feet*(1-p.death)+p.death*.10f,b.pos.z);rlRotatef(p.yaw*RAD2DEG,0,1,0);
        rlTranslatef((lean+std::sin(phase)*.012f*motion)*(1-collapse),bounce*(1-collapse),0);
        Color fabric{110,121,91,255},pants{148,143,113,255},armor{69,80,66,255},web{101,111,80,255},skin{178,143,110,255};
        // Feet travel through an alternating planted/swing cycle; knees bend forward.
        for(int side=-1;side<=1;side+=2){
            float q=phase+(side<0?PI:0),swing=std::sin(q),lift=std::max(0.f,std::cos(q))*(.10f+.12f*running)*motion*(1-airborne);
            Vector3 hip{side*.105f,.94f-.55f*crouch,0},knee{side*.105f,.49f-.15f*crouch-lift*.2f,-.045f-.25f*crouch-.18f*airborne-std::max(0.f,swing)*(.12f+.1f*running)*motion};
            Vector3 ankle{side*.11f,.10f+lift,swing*(.20f+.13f*running)*motion*(1-airborne)};
            hip.y=hipY;
            knee=Vector3Lerp(knee,{side*(.18f+.025f*(index%2)),.13f,-p.deathDirection*.38f},collapse);
            ankle=Vector3Lerp(ankle,{side*.22f,.10f,-p.deathDirection*(.72f+(side<0?.07f:0))},collapse);
            segment(g,hip,knee,.106f,.107f,pants);segment(g,knee,ankle,.078f,.080f,pants);
            g.sphere(knee,{.078f,.095f,.080f},armor);
            g.box({knee.x,knee.y,knee.z-.072f},{.095f,.12f,.032f},{80,87,70,255});
            g.mesh(boot,{ankle.x,ankle.y-.10f,ankle.z},{.92f,.85f,.94f},{48,49,42,255});
            for(int n=0;n<4;n++)g.box({ankle.x,.10f+lift+n*.018f,ankle.z-.095f},{.09f,.008f,.012f},{89,87,66,255});
            g.box({side*.155f,hipY-.17f,.02f},{.065f,.18f,.13f},web);
        }
        rlPushMatrix();rlTranslatef(0,hipY,0);
        rlRotatef(fall.torsoPitch-p.hit*4-running*5*(1-collapse),1,0,0);rlRotatef(fall.torsoRoll+std::sin(phase)*1.4f*motion*(1-collapse),0,0,1);
        rlTranslatef(0,-.94f,0);
        garment(g,torso,{0,.92f,0},{1,1,1},fabric);
        // Shaped carrier front/back, straps, magazines, belt equipment and radio.
        g.mesh(torso,{0,.995f,-.055f},{1.05f,.76f,1.2f},armor);
        g.box({0,.95f,0},{.37f,.065f,.25f},{48,58,47,255});
        for(float x:{-.13f,-.043f,.043f,.13f}){
            g.mesh(pouch,{x,1.08f,-.205f},{.90f,1.05f,1},web);
            g.box({x,1.205f,-.235f},{.05f,.022f,.018f},{144,142,104,255});
        }
        for(float x:{-.155f,.155f}){
            g.box({x,1.40f,-.115f},{.052f,.14f,.09f},web);
            g.box({x,1.38f,-.166f},{.037f,.045f,.014f},{153,148,107,255});
        }
        g.box({.19f,1.25f,.11f},{.09f,.19f,.07f},{43,55,45,255});
        g.link({.19f,1.34f,.11f},{.19f,1.61f,.11f},.005f,{35,42,36,255});
        g.box({-.18f,.98f,.09f},{.10f,.10f,.08f},web);
        g.mesh(pouch,{.21f,.93f,.02f},{1.0f,1.0f,1.1f},armor);
        // Elbows follow a relaxed two-handed aiming pose, with torso sway/recoil.
        float recoil=b.shotFlash>0?.025f:0;
        Vector3 rightShoulder{.235f,1.43f,0},rightElbow{.29f,1.17f,-.11f},rightHand{.105f,1.29f,-.33f+recoil};
        Vector3 leftShoulder{-.235f,1.43f,0},leftElbow{-.29f,1.17f,-.19f},leftHand{-.035f,1.29f,-.55f+recoil};
        rightElbow=Vector3Lerp(rightElbow,{.36f,1.24f,.02f*p.deathDirection},fall.armRelease);
        leftElbow=Vector3Lerp(leftElbow,{-.38f,1.18f,.04f*p.deathDirection},fall.armRelease);
        rightHand=Vector3Lerp(rightHand,{.42f,1.02f,-.06f*p.deathDirection},fall.armRelease);
        leftHand=Vector3Lerp(leftHand,{-.40f,.99f,-.08f*p.deathDirection},fall.armRelease);
        for(int side:{-1,1}){
            Vector3 shoulder=side<0?leftShoulder:rightShoulder,elbow=side<0?leftElbow:rightElbow,hand=side<0?leftHand:rightHand;
            segment(g,shoulder,elbow,.082f,.080f,fabric);segment(g,elbow,hand,.066f,.064f,fabric);
            g.sphere(elbow,{.072f,.072f,.073f},armor);
            g.sphere(hand,{.043f,.052f,.057f},{53,61,48,255});
            for(int finger=0;finger<4;finger++)g.link({hand.x-.03f+finger*.018f,hand.y-.018f,hand.z-.025f},{hand.x-.03f+finger*.018f,hand.y-.035f,hand.z-.055f},.008f,{76,82,61,255});
            g.box({shoulder.x*1.1f,1.38f,-.025f},{.014f,.09f,.085f},{191,124,43,255});
        }
        // Anatomical head rather than a spherical doll face.
        g.link({0,1.45f,0},{0,1.58f,0},.055f,skin);
        rlPushMatrix();rlTranslatef(0,breathe,0);rlRotatef(std::sin(elapsed*.9f+index)*2*(1-collapse),0,1,0);
        g.mesh(head,{0,1.53f,0},{1,1,1},skin);
        for(float side:{-1.f,1.f}){
            g.sphere({side*.105f,1.67f,.005f},{.019f,.035f,.022f},skin);
            g.sphere({side*.039f,1.695f,-.087f},{.023f,.009f,.008f},{220,214,191,255});
            g.sphere({side*.039f,1.695f,-.094f},{.008f,.008f,.004f},{43,54,44,255});
            g.box({side*.039f,1.717f,-.088f},{.047f,.009f,.012f},{73,62,44,255});
        }
        g.sphere({0,1.668f,-.105f},{.017f,.032f,.022f},skin);
        g.sphere({0,1.648f,-.121f},{.022f,.013f,.015f},{166,128,96,255});
        g.box({0,1.615f,-.084f},{.042f,.007f,.009f},{123,83,66,255});
        // Helmet cloth shell, ear protection, retention straps and goggles.
        g.mesh(helmet,{0,1.715f,.008f},{1,1,1},{94,106,78,255});
        g.box({0,1.738f,-.113f},{.207f,.026f,.045f},{65,79,63,255});
        for(float side:{-1.f,1.f}){
            g.sphere({side*.121f,1.692f,.012f},{.028f,.047f,.035f},{52,64,52,255});
            g.link({side*.10f,1.72f,-.01f},{side*.08f,1.56f,-.05f},.007f,{53,66,52,255});
            g.box({side*.05f,1.695f,-.104f},{.083f,.038f,.03f},{40,52,44,255});
            g.box({side*.05f,1.697f,-.122f},{.065f,.023f,.006f},{114,149,132,255});
        }
        g.link({-.135f,1.68f,.005f},{-.075f,1.625f,-.115f},.005f,{35,44,39,255});
        rlPopMatrix();
        // Weapon leaves the hands as the arms release, then settles beside the body.
        if(p.death<.22f){
        // Original compact rifle with barrel, stock, magazine, sights and support grip.
        g.box({.07f,1.305f,-.38f+recoil},{.085f,.085f,.37f},{44,50,47,255});
        g.box({.07f,1.245f,-.29f+recoil},{.06f,.16f,.10f},{39,45,42,255},-1,8);
        g.link({.07f,1.32f,-.53f+recoil},{.07f,1.32f,-.87f+recoil},.016f,{29,35,33,255});
        g.box({.07f,1.315f,-.10f},{.07f,.10f,.17f},{51,58,49,255});
        for(int n=0;n<7;n++)g.box({.07f,1.358f,-.24f-n*.035f},{.07f,.012f,.014f},{81,88,76,255});
        if(b.selected==strike::WeaponId::AW9){
            g.link({.07f,1.32f,-.75f},{.07f,1.32f,-.87f},.019f,{29,35,33,255});
            g.link({.07f,1.41f,-.21f},{.07f,1.41f,-.48f},.036f,{35,43,41,255});
            g.sphere({.07f,1.41f,-.49f},{.032f,.032f,.012f},{79,118,125,255});
        }
        if(b.shotFlash>0&&!g.shadowPass)g.sphere({.07f,1.32f,-.91f},{.045f,.045f,.10f},{255,220,134,255});
        }
        rlPopMatrix();
        if(p.death>=.22f){
            float drop=strike::poseBlend(p.death,.22f,.7f);
            rlPushMatrix();rlTranslatef(.38f,.075f+(1-drop)*.45f,-p.deathDirection*.25f);
            rlRotatef(22+index*13,0,1,0);rlRotatef(90,0,0,1);
            g.box({0,0,0},{.085f,.085f,.50f},{44,50,47,255});
            g.link({0,0,-.25f},{0,0,-.58f},.016f,{29,35,33,255});
            g.box({0,-.08f,.08f},{.06f,.16f,.10f},{39,45,42,255});
            rlPopMatrix();
        }
        rlPopMatrix();
    }
};
