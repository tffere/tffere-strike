#pragma once
// Original procedural first-person equipment. Dimensions are metres.
struct WeaponRenderer {
    Model bevel{}, sleeve{}, palm{}, magazine{};
    static Model loft(const std::vector<Vector3>& rings,int sides=12,bool smooth=false) {
        // Each ring stores elliptical x/y radius and position along z.
        std::vector<float> v,n,uv;
        auto emit=[&](Vector3 p,Vector3 normal,float u,float t){v.insert(v.end(),{p.x,p.y,p.z});n.insert(n.end(),{normal.x,normal.y,normal.z});uv.insert(uv.end(),{u,t});};
        for(size_t k=0;k+1<rings.size();++k)for(int j=0;j<sides;++j){
            float a=2*PI*j/sides,b=2*PI*(j+1)/sides;
            auto point=[](Vector3 r,float q){return Vector3{r.x*std::cos(q),r.y*std::sin(q),r.z};};
            Vector3 p[4]={point(rings[k],a),point(rings[k],b),point(rings[k+1],b),point(rings[k+1],a)};
            Vector3 nn=Vector3Normalize(Vector3CrossProduct(Vector3Subtract(p[1],p[0]),Vector3Subtract(p[2],p[0])));
            for(int q:{0,1,2,0,2,3}){
                Vector3 normal=nn;
                if(smooth){Vector3 r=rings[(q<2)?k:k+1];normal=Vector3Normalize(Vector3{p[q].x/(r.x*r.x),p[q].y/(r.y*r.y),0});}
                emit(p[q],normal,float(j)/sides,float(k)/(rings.size()-1));
            }
        }
        for(int end=0;end<2;++end){Vector3 r=rings[end?rings.size()-1:0];for(int j=0;j<sides;++j){float a=2*PI*j/sides,b=2*PI*(j+1)/sides;Vector3 normal{0,0,end?1.f:-1.f};emit({0,0,r.z},normal,.5,.5);emit({r.x*std::cos(end?a:b),r.y*std::sin(end?a:b),r.z},normal,0,0);emit({r.x*std::cos(end?b:a),r.y*std::sin(end?b:a),r.z},normal,1,1);}}
        Mesh m{};m.vertexCount=int(v.size()/3);m.triangleCount=m.vertexCount/3;
        m.vertices=(float*)MemAlloc(v.size()*sizeof(float));m.normals=(float*)MemAlloc(n.size()*sizeof(float));m.texcoords=(float*)MemAlloc(uv.size()*sizeof(float));
        std::copy(v.begin(),v.end(),m.vertices);std::copy(n.begin(),n.end(),m.normals);std::copy(uv.begin(),uv.end(),m.texcoords);UploadMesh(&m,false);return LoadModelFromMesh(m);
    }
    static Model chamfer() {
        const Vector2 p[8]={{-.36f,-.5f},{.36f,-.5f},{.5f,-.33f},{.5f,.33f},{.36f,.5f},{-.36f,.5f},{-.5f,.33f},{-.5f,-.33f}};
        std::vector<float> v,n,uv;
        auto emit=[&](Vector3 q,Vector3 normal){v.insert(v.end(),{q.x,q.y,q.z});n.insert(n.end(),{normal.x,normal.y,normal.z});uv.insert(uv.end(),{q.z+.5f,q.y+.5f});};
        for(int i=0;i<8;++i){int j=(i+1)%8;Vector3 a{p[i].x,p[i].y,-.5f},b{p[j].x,p[j].y,-.5f},c{p[j].x,p[j].y,.5f},d{p[i].x,p[i].y,.5f};Vector3 normal=Vector3Normalize(Vector3{p[j].y-p[i].y,p[i].x-p[j].x,0});for(auto q:{a,b,c,a,c,d})emit(q,normal);}
        for(int e=0;e<2;++e)for(int i=0;i<8;++i){int j=(i+1)%8;float z=e?.5f:-.5f;emit({0,0,z},{0,0,e?1.f:-1.f});emit({p[e?i:j].x,p[e?i:j].y,z},{0,0,e?1.f:-1.f});emit({p[e?j:i].x,p[e?j:i].y,z},{0,0,e?1.f:-1.f});}
        Mesh m{};m.vertexCount=int(v.size()/3);m.triangleCount=m.vertexCount/3;m.vertices=(float*)MemAlloc(v.size()*sizeof(float));m.normals=(float*)MemAlloc(n.size()*sizeof(float));m.texcoords=(float*)MemAlloc(uv.size()*sizeof(float));std::copy(v.begin(),v.end(),m.vertices);std::copy(n.begin(),n.end(),m.normals);std::copy(uv.begin(),uv.end(),m.texcoords);UploadMesh(&m,false);return LoadModelFromMesh(m);
    }
    WeaponRenderer(){bevel=chamfer();sleeve=loft({{.88f,.79f,-.5f},{1,.87f,-.25f},{.94f,.83f,-.06f},{.76f,.73f,.23f},{.61f,.62f,.42f},{.61f,.61f,.5f}},32,true);palm=loft({{.46f,.52f,-.5f},{.88f,.79f,-.27f},{1,.84f,.12f},{.87f,.69f,.4f},{.60f,.51f,.5f}},24,true);magazine=loft({{.42f,.45f,-.5f},{.48f,.5f,-.35f},{.50f,.5f,.25f},{.43f,.45f,.5f}},8);}
    ~WeaponRenderer(){UnloadModel(bevel);UnloadModel(sleeve);UnloadModel(palm);UnloadModel(magazine);}
    template<class G>void draw(G&g,bool sniper,float elapsed,float recoil,float reload){
        const Color metal{52,59,61,255}, edge{78,85,86,255}, black{24,29,30,255}, rubber{35,39,37,255}, steel{104,111,109,255}, fabric{181,142,108,255}, seam{30,32,32,255}, glove{28,29,29,255}, leather{20,23,24,255};
        auto part=[&](Vector3 p,Vector3 s,Color c,float a=0){g.mesh(bevel,p,s,c,-1,{1,0,0},a);};
        auto capsule=[&](Vector3 a,Vector3 b,float r,Color c){g.link(a,b,r,c);g.sphere(a,{r,r,r},c);g.sphere(b,{r,r,r},c);};
        auto limb=[&](Model&m,Vector3 a,Vector3 b,float x,float y,Color c){Vector3 d=Vector3Subtract(b,a),axis=Vector3CrossProduct({0,0,1},Vector3Normalize(d));if(Vector3Length(axis)<.001f)axis={0,1,0};g.mesh(m,Vector3Scale(Vector3Add(a,b),.5f),{x,y,Vector3Length(d)},c,-1,axis,Vector3Angle({0,0,1},d)*RAD2DEG);};
        rlPushMatrix();rlTranslatef(.235f+std::sin(elapsed*1.8f)*.0017f,-.225f-reload*.25f,-.57f+recoil*.075f);rlRotatef(-recoil*6+reload*32,1,0,0);rlRotatef(reload*16,0,0,1);
        // Tailored sleeves taper into tight ribbed cuffs; anatomical palms and separate digits.
        limb(sleeve,{.145f,-.28f,.37f},{.035f,-.135f,.10f},.066f,.061f,fabric);
        // Bent support arm enters from below the receiver rather than crossing the screen as a straight beam.
        limb(sleeve,{-.125f,-.355f,.015f},{-.055f,-.116f,-.283f},.053f,.049f,fabric);
        limb(palm,{-.055f,-.116f,-.283f},{-.053f,-.096f,-.315f},.034f,.030f,seam);
        // Folded cuff has overlapping fabric bands and a small fastening tab.
        for(int i=0;i<3;++i){float t=i*.006f;limb(palm,{-.056f,-.119f-t,-.279f+t},{-.054f,-.111f-t,-.292f+t},.036f+i*.0008f,.033f,seam);}
        part({-.080f,-.106f,-.293f},{.014f,.018f,.031f},glove,18);
        // Two shallow cloth folds follow the bend at the wrist, not the whole arm.
        limb(palm,{.035f,-.137f,.11f},{.025f,-.082f,.025f},.042f,.037f,glove);
        limb(palm,{-.053f,-.098f,-.311f},{-.033f,-.067f,-.365f},.036f,.030f,glove);
        for(int i=0;i<3;++i){float t=i*.009f;capsule({.012f+t,-.15f,.115f},{.054f+t,-.128f,.09f},.003f,seam);}
        // Trigger hand knuckles follow the sloping pistol grip.
        for(int i=0;i<3;++i){float y=-.095f-i*.016f;capsule({.039f,y,.036f},{.016f,y-.003f,.011f},.010f,glove);capsule({.016f,y-.003f,.011f},{-.012f,y-.007f,.026f},.0085f,leather);part({.037f,y+.003f,.045f},{.018f,.009f,.017f},leather);}
        capsule({.039f,-.073f,.032f},{.031f,-.061f,-.012f},.008f,glove);capsule({.031f,-.061f,-.012f},{.011f,-.068f,-.035f},.007f,leather);
        capsule({-.025f,-.076f,.061f},{-.034f,-.049f,.009f},.013f,glove);
        for(int i=0;i<4;++i){float z=-.335f-i*.019f;capsule({-.047f,-.062f,z},{-.030f,-.082f,z},.0085f,glove);capsule({-.030f,-.082f,z},{.015f,-.078f,z},.008f,leather);capsule({.015f,-.078f,z},{.030f,-.059f,z},.007f,glove);}
        capsule({-.046f,-.055f,-.335f},{-.039f,-.013f,-.372f},.011f,glove);
        if(sniper){
            part({0,-.026f,-.13f},{.083f,.078f,.59f},{87,88,65,255});part({0,-.018f,.225f},{.091f,.117f,.28f},{77,81,61,255});part({0,.031f,.225f},{.087f,.028f,.18f},rubber);part({0,-.015f,.375f},{.094f,.12f,.022f},rubber);
            g.link({0,.019f,.14f},{0,.019f,-.28f},.027f,metal);g.link({0,.019f,-.28f},{0,.019f,-.89f},.015f,black);g.link({0,.019f,-.84f},{0,.019f,-.93f},.023f,metal);
            part({0,-.098f,.026f},{.052f,.12f,.075f},rubber,-16);part({0,-.099f,-.13f},{.065f,.092f,.10f},metal);part({0,-.148f,-.13f},{.069f,.012f,.106f},black);
            for(float z:{-.13f,.06f}){part({0,.06f,z},{.042f,.044f,.029f},metal);g.link({0,.11f,z-.012f},{0,.11f,z+.012f},.033f,edge);}
            g.link({0,.11f,.18f},{0,.11f,-.31f},.024f,black);g.link({0,.11f,.12f},{0,.11f,.20f},.035f,rubber);g.link({0,.11f,-.27f},{0,.11f,-.39f},.045f,black);
            g.link({0,.11f,.201f},{0,.11f,.203f},.027f,{38,73,78,255});g.link({0,.11f,-.391f},{0,.11f,-.393f},.037f,{32,69,72,255});
            for(int i=0;i<6;++i){float z=.125f+i*.011f;g.link({0,.11f,z},{0,.11f,z+.003f},.036f,edge);}
            g.link({0,.128f,-.07f},{0,.161f,-.07f},.017f,metal);g.link({.016f,.11f,-.07f},{.041f,.11f,-.07f},.016f,metal);
            g.link({.022f,.015f,.085f},{.069f,-.013f,.11f},.009f,steel);g.sphere({.071f,-.015f,.112f},{.015f,.015f,.015f},black);
            for(float s:{-1.f,1.f}){g.link({s*.037f,-.047f,-.35f},{s*.043f,-.058f,-.57f},.007f,metal);part({s*.045f,-.029f,-.25f},{.005f,.026f,.26f},rubber);}
        }else{
            // Forged upper and lower receiver, sloped magazine well and thin ejection opening.
            part({0,.012f,-.025f},{.078f,.079f,.275f},metal);part({0,-.037f,-.012f},{.069f,.045f,.232f},metal);part({0,-.050f,-.112f},{.074f,.065f,.078f},edge,-7);
            part({.040f,.018f,-.038f},{.003f,.026f,.095f},black);part({.042f,.004f,-.041f},{.005f,.007f,.101f},edge);part({.043f,.026f,.036f},{.009f,.014f,.030f},edge);
            part({0,.004f,-.32f},{.076f,.075f,.31f},black);part({0,.043f,-.31f},{.040f,.011f,.31f},edge);
            for(int i=0;i<7;++i){float z=-.20f-i*.034f;part({.039f,.005f,z},{.003f,.016f,.022f},metal);part({-.039f,.005f,z},{.003f,.016f,.022f},metal);part({0,-.035f,z},{.029f,.003f,.019f},metal);}
            g.link({0,.004f,-.47f},{0,.004f,-.74f},.0105f,metal);g.link({0,.004f,-.72f},{0,.004f,-.785f},.017f,black);for(int i=0;i<3;++i)part({.017f,.004f,-.737f-i*.012f},{.003f,.010f,.007f},edge);
            g.link({0,-.005f,.11f},{0,-.005f,.27f},.017f,metal);part({0,-.029f,.245f},{.074f,.105f,.19f},black);part({0,.010f,.235f},{.075f,.034f,.155f},rubber);part({0,-.032f,.345f},{.079f,.123f,.018f},rubber);
            part({0,-.102f,.04f},{.051f,.125f,.074f},rubber,-17);for(int i=0;i<4;++i)part({.027f,-.075f-i*.017f,.044f},{.003f,.004f,.047f},metal,-17);
            rlPushMatrix();rlTranslatef(0,-.151f,-.121f);rlRotatef(-8,1,0,0);g.mesh(magazine,{0,0,0},{.067f,.108f,.185f},metal,-1,{1,0,0},90);for(float s:{-1.f,1.f})for(int i=0;i<3;++i)part({s*.031f,-.005f,.025f-i*.025f},{.004f,.115f,.006f},black);part({0,-.088f,0},{.073f,.012f,.114f},black);rlPopMatrix();
            // Picatinny teeth have shallow dimensions; compact protected rear aperture.
            part({0,.056f,-.125f},{.037f,.009f,.46f},black);for(int i=0;i<21;++i)part({0,.063f,.083f-i*.022f},{.043f,.007f,.010f},edge);
            for(float s:{-1.f,1.f})part({s*.013f,.082f,.068f},{.008f,.025f,.025f},black);part({0,.095f,.068f},{.029f,.006f,.014f},metal);part({0,.077f,.068f},{.029f,.005f,.014f},metal);
            part({0,.075f,-.458f},{.007f,.029f,.012f},black);for(float s:{-1.f,1.f})part({s*.014f,.071f,-.458f},{.006f,.025f,.018f},metal);
            // Pins, forward assist and selector occupy the receiver's exposed right side.
            for(float z:{-.096f,.073f})g.sphere({.039f,-.023f,z},{.004f,.004f,.004f},steel);
            g.link({.041f,-.033f,.041f},{.046f,-.041f,.061f},.004f,black);g.link({.039f,.027f,.065f},{.056f,.028f,.085f},.007f,metal);
        }
        // Slim rounded trigger guard and visible curved trigger, shared by both platforms.
        capsule({-.023f,-.063f,.004f},{-.023f,-.091f,-.013f},.0035f,metal);capsule({-.023f,-.091f,-.013f},{-.023f,-.092f,-.053f},.0035f,metal);capsule({-.023f,-.092f,-.053f},{-.023f,-.058f,-.066f},.0035f,metal);capsule({0,-.057f,-.028f},{0,-.079f,-.036f},.003f,steel);
        rlPopMatrix();
    }
};
