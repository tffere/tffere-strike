#pragma once
#include <cmath>
// Original desert architecture: three connected routes, two courts and a covered arcade.
template<class G> void buildDesertMap(G& g,const strike::Game& game) {
    g.props.clear();
    const Color sand{222,204,173,255}, pale{239,222,190,255}, dark{83,77,65,255};
    const Color teal{56,92,89,255}, wood{139,102,60,255};
    g.add(0,-.17f,0,66,.3f,66,{205,188,156,255},1);
    // Worn stone paving follows the actual playable routes.
    g.add(0,.006f,0,9,.018f,61,{216,204,181,255},1);
    g.add(25,.009f,1,10,.018f,59,{211,199,175,255},1);
    g.add(-25,.01f,0,10,.018f,60,{207,195,171,255},1);
    g.add(0,.012f,-27,60,.018f,7,{218,205,179,255},1);
    g.add(0,.014f,18,60,.018f,3,{209,197,172,255},1);
    int building=0;
    for(const auto& o:game.obstacles) {
        using K=strike::ObstacleKind;
        if(o.kind==K::Barrel) {
            g.pipe({o.x,.05f,o.z},{o.x,o.h,o.z},o.w*.48f,teal);
            for(float y:{.15f,o.h*.5f,o.h-.12f}) g.pipe({o.x,y,o.z},{o.x,y+.065f,o.z},o.w*.495f,{65,65,55,255});
            continue;
        }
        if(o.kind==K::Crate) {
            g.add(o.x,o.h*.5f,o.z,o.w,o.h,o.d,wood,2);
            for(float x:{o.x-o.w*.42f,o.x+o.w*.42f}) g.add(x,o.h*.5f,o.z+o.d*.5f+.025f,.12f,o.h,.08f,dark,2);
            for(float y:{.13f,o.h-.13f})g.add(o.x,y,o.z+o.d*.5f+.04f,o.w,.16f,.1f,dark,2);
            continue;
        }
        bool house=o.kind==K::Building;
        Color c=house?Color{(unsigned char)(225+(building%3)*5),(unsigned char)(209+(building%3)*4),(unsigned char)(179+(building%3)*5),255}:sand;
        g.add(o.x,o.h*.5f,o.z,o.w,o.h,o.d,c,0);
        if(!house) { if(o.w>8 || o.d>8)g.add(o.x,o.h+.12f,o.z,o.w+.1f,.24f,o.d+.1f,pale);continue; }
        ++building;
        g.add(o.x,.25f,o.z,o.w+.12f,.5f,o.d+.12f,{186,172,146,255},0);
        g.add(o.x,o.h-.18f,o.z,o.w+.28f,.32f,o.d+.28f,pale);
        // Roof parapet, with an inset flat roof visible from the spectator camera.
        g.add(o.x,o.h+.30f,o.z-o.d*.5f+.16f,o.w,.6f,.32f,pale);
        g.add(o.x,o.h+.30f,o.z+o.d*.5f-.16f,o.w,.6f,.32f,pale);
        g.add(o.x-o.w*.5f+.16f,o.h+.3f,o.z,.32f,.6f,o.d,pale);
        g.add(o.x+o.w*.5f-.16f,o.h+.3f,o.z,.32f,.6f,o.d,pale);
        // Shuttered windows and masonry surrounds on both long-facing facades.
        for(float x=o.x-o.w*.5f+2;x<o.x+o.w*.5f-1;x+=3.1f)for(int side:{-1,1}){
            float z=o.z+side*(o.d*.5f+.035f);
            g.add(x,4.4f,z,1.35f,1.75f,.1f,dark,2);
            g.add(x,4.4f,z+side*.065f,1.13f,1.5f,.06f,teal,3);
            g.add(x,3.49f,z,1.65f,.17f,.3f,pale);
            g.add(x,5.30f,z,1.55f,.18f,.23f,pale);
            g.add(x,4.4f,z+side*.11f,.055f,1.5f,.045f,dark);
        }
        for(int side:{-1,1}){
            float x=o.x+side*(o.w*.5f+.03f);
            for(float z=o.z-o.d*.5f+2;z<o.z+o.d*.5f-1;z+=4.f){
                g.add(x,4.3f,z,.1f,1.55f,1.2f,dark,2);
                g.add(x+side*.06f,4.3f,z,.06f,1.35f,1.0f,teal,3);
                g.add(x,3.48f,z,.3f,.16f,1.5f,pale);
                g.add(x,5.12f,z,.23f,.17f,1.5f,pale);
                for(float dz:{-.65f,.65f})g.add(x,4.3f,z+dz,.2f,1.6f,.13f,pale);
            }
        }
        float front=o.z+o.d*.5f+.06f;
        g.add(o.x,1.5f,front,1.9f,3,.13f,dark,2);
        g.add(o.x,1.45f,front+.08f,1.65f,2.8f,.07f,wood,2);
        for(float x:{o.x-1.2432f,o.x+1.2432f})g.add(x,1.525f,front+.16f,.2464f,3.05f,.34f,pale);
        g.add(o.x+.52f,1.42f,front+.14f,.1f,.13f,.06f,{54,45,31,255});
        // Continuous extruded masonry joins the full-height jambs without floating stone gaps.
        g.add(o.x,3.34f,front+.025f,1.75f,.38f,.11f,{67,72,63,255});
        g.arch(o.x,3.05f,front+.16f,1.12f,.85f,.34f,pale);
        // Flaking lime plaster exposes older, darker repairs without obstructing a route.
        for(int k=0;k<3;k++) {
            float px=o.x-o.w*.37f+k*o.w*.32f;
            g.add(px,.9f+(k%2)*.6f,front-.025f,1.1f+k*.2f,.35f,.07f,{215,196,162,255});
        }
        // A projecting lamp is anchored to the wall, above head height.
        g.pipe({o.x+1.7f,3.1f,front},{o.x+1.7f,3.1f,front+.42f},.045f,dark);
        g.add(o.x+1.7f,2.94f,front+.42f,.30f,.35f,.28f,{55,64,58,255},3);
        g.add(o.x+1.7f,2.94f,front+.58f,.2f,.22f,.035f,{239,205,130,255});
        g.add(o.x+1.7f,3.16f,front+.42f,.4f,.09f,.38f,dark);
        // Human-scale side entrances face the lanes; their doors remain closed against collision walls.
        for(int side : {o.x<0?1:-1}) {
            float sx=o.x+side*(o.w*.5f+.045f),sz=o.z+o.d*.5f-2.6f;
            g.add(sx,1.35f,sz,.10f,2.7f,1.45f,{67,82,78,255},3);
            for(float dz:{-.83f,.83f})g.add(sx+side*.04f,1.45f,sz+dz,.25f,2.9f,.16f,pale);
            g.add(sx,2.95f,sz,.3f,.2f,1.95f,pale);
            g.add(sx+side*.42f,3.22f,sz,.94f,.12f,2.2f,{135,157,145,255},3);
            g.add(sx+side*.85f,3.08f,sz,.10f,.3f,2.2f,{105,131,121,255});
            // An inset stone panel and iron balcony give each lane-facing wall depth.
            float wz=o.z-o.d*.20f;
            g.add(sx,1.85f,wz,.1f,1.1f,1.35f,{116,139,125,255},3);
            g.add(sx,1.24f,wz,.28f,.15f,1.6f,pale);
            g.add(sx+side*.15f,2.45f,wz,.25f,.16f,1.55f,pale);
            g.add(sx+side*.23f,3.53f,wz,.55f,.17f,2.0f,pale);
            g.pipe({sx+side*.45f,4.05f,wz-.95f},{sx+side*.45f,4.05f,wz+.95f},.04f,dark);
            for(float dz:{-.9f,0.f,.9f})g.pipe({sx+side*.45f,3.58f,wz+dz},{sx+side*.45f,4.05f,wz+dz},.025f,dark);
        }
        // Visible utility drainpipe along a corner.
        g.pipe({o.x+o.w*.5f+.12f,.2f,o.z+o.d*.5f},{o.x+o.w*.5f+.12f,o.h-.5f,o.z+o.d*.5f},.08f,{109,100,77,255});
    }
    // The boundary reads as an inhabited old-quarter frontage rather than a blank arena box.
    // Shallow masonry stays within the original wall's character-clearance margin.
    for(int side:{-1,1}) {
        float x=side*30.94f;
        g.add(x,.3f,0,.18f,.6f,61,{184,172,149,255},0);
        for(float z:{-24.f,-12.f,0.f,12.f,24.f}) {
            g.add(x,3,z,.24f,6,.42f,{218,202,174,255},0);
            g.add(x,5.75f,z,.32f,.3f,.7f,pale);
        }
        for(float z:{-18.f,-3.f,15.f}) {
            float face=x-side*.1f;
            g.add(face,1.7f,z,.10f,2.1f,1.65f,{78,85,71,255});
            g.add(face-side*.055f,1.7f,z,.045f,1.9f,1.45f,{108,130,115,255},3);
            for(float dz:{-.91f,.91f})g.add(face,1.7f,z+dz,.19f,2.35f,.17f,pale);
            g.add(face,2.94f,z,.23f,.22f,2.05f,pale);
            g.add(face,.55f,z,.27f,.15f,2.05f,pale);
            g.add(face-side*.09f,1.7f,z,.04f,1.9f,.075f,dark);
            g.add(face-side*.1f,1.26f,z,.04f,.085f,1.45f,dark);
        }
        // A striped high canopy gives each long wall a recognizable market bay.
        float ax=side*29.5f,az=side>0?19.f:18.f;
        g.add(ax,3.3f,az,3,.12f,3.1f,{124,143,128,255});
        for(int k=0;k<4;k++) {
            float z=az-1.2f+k*.8f;
            g.add(ax,3.37f,z,3,.025f,.36f,{224,211,178,255});
            g.add(side*28.03f,3.16f,z,.075f,.32f,.36f,{224,211,178,255});
        }
    }
    // Matching closed shop doors also face the covered passage and long road.
    for(int side:{-1,1}) {
        float x=side*19.05f,z=side<0?7.f:9.f;
        g.add(x,1.35f,z,.1f,2.7f,1.55f,{88,119,108,255},3);
        for(float dz:{-.86f,.86f})g.add(x,1.4f,z+dz,.24f,2.8f,.17f,pale);
        g.add(x,2.9f,z,.28f,.2f,2.0f,pale);
        g.add(x+side*.4f,3.2f,z,.95f,.13f,2.2f,{121,148,132,255},3);
    }
    // Sagging overhead utility wires link the eastern houses to their perimeter frontage.
    for(float z:{-12.f,8.f,22.f}) {
        float start=z>20?22.f:19.f;
        g.pipe({start,5.5f,z},{24,5.1f,z+.12f},.023f,{60,65,58,255});
        g.pipe({24,5.1f,z+.12f},{28,5.15f,z+.18f},.023f,{60,65,58,255});
        g.pipe({28,5.15f,z+.18f},{31,5.6f,z},.023f,{60,65,58,255});
    }
    // Long covered tunnel arcade. Supports are collision boxes in the layout.
    for(float z:{14.f,7.f,0.f,-7.f}){
        g.add(-25,4.15f,z,10,.65f,1.5f,pale);
        for(float x:{-28.7f,-21.3f})g.add(x,3.65f,z,1.5f,.4f,1.35f,pale);
    }
    g.add(-25,4.65f,3.5f,10,.4f,24,{173,144,103,255},0);
    // Mid's two weathered wooden leaves and high stone arch.
    for(float x:{-3.6f,3.6f}){
        g.add(x,1.7f,9.36f,2.62f,3.3f,.12f,wood,2);
        for(float y:{.6f,2.7f})g.add(x,y,9.45f,2.65f,.16f,.08f,{72,66,52,255});
    }
    g.add(0,5.3f,9,10,.7f,1.1f,pale);
    g.arch(0,3.1f,9,2.6f,1.8f,1.02f,pale);
    for(float x:{-2.886f,2.886f})g.add(x,1.55f,9,.572f,3.1f,1.02f,pale);
    // Spandrels overlap only the outer stone rim, preserving the complete curved opening.
    constexpr float outerWidth=2.6f*1.22f,outerRise=1.8f*1.22f;
    for(int k=0;k<16;k++) {
        float w=outerWidth*2/16;
        float x=-outerWidth+(k+.5f)*w;
        float u=std::min(1.f,(std::abs(x)+w*.5f)/outerWidth);
        float bottom=3.1f+outerRise*std::sqrt(std::max(0.f,1-u*u))-.025f;
        float h=5.35f-bottom;
        if(h>.025f)g.add(x,bottom+h*.5f,9,w+.01f,h,1.08f,pale);
    }
    for(float x:{-4.8f,4.8f}) {
        g.add(x,4.3f,9,.4f,1.6f,1.12f,pale,0);
        g.add(x,5.43f,9,.7f,.3f,1.35f,pale);
    }
    // Market awning above the west route, decorative poles outside the walkway.
    g.add(-22.4f,3.25f,17.5f,5.5f,.12f,2.7f,{137,82,59,255});
    for(int k=0;k<7;k++) {
        float x=-24.8f+k*.8f;
        g.add(x,3.32f,17.5f,.39f,.025f,2.7f,{213,184,136,255});
        g.add(x,3.1f,18.87f,.39f,.35f,.06f,{213,184,136,255});
    }
    // Cantilever braces anchor into the south-house wall; no decorative pole occupies the walking lane.
    for(float x:{-24.9f,-20.f})g.pipe({x,2.85f,20.03f},{x,3.3f,18.6f},.065f,wood);
    // Roof tanks, antennae, air-conditioning and wall lights establish scale.
    for(auto p:std::array<Vector3,4>{{{-12,7.7f,4},{12,8.3f,7},{-18,6.7f,24},{12,7.3f,-11}}}){
        g.add(p.x,p.y+.3f,p.z,1.4f,.65f,1.25f,{148,153,139,255},3);
        g.pipe({p.x+1.5f,p.y,p.z},{p.x+1.5f,p.y+2.6f,p.z},.035f,dark);
        g.pipe({p.x+.7f,p.y+2.3f,p.z},{p.x+2.3f,p.y+2.3f,p.z},.025f,dark);
    }
    auto palm=[&](float x,float z){
        g.pipe({x,0,z},{x+.16f,3,z+.08f},.24f,{119,91,57,255});
        g.pipe({x+.16f,3,z+.08f},{x+.4f,6.2f,z},.18f,{130,101,64,255});
        for(int i=0;i<7;i++) {
            float a=i*.8976f;
            Vector3 mid{x+.4f+std::cos(a)*1.4f,6.45f,z+std::sin(a)*1.4f};
            Vector3 tip{x+.4f+std::cos(a)*3.0f,5.25f,z+std::sin(a)*3.0f};
            g.pipe({x+.4f,6.2f,z},mid,.10f,{63,88,43,255});
            g.pipe(mid,tip,.09f,{63,88,43,255});
            // Tapered leaflets turn the flat radial silhouette into drooping fronds.
            for(float t:{.35f,.7f}) {
                Vector3 p{mid.x+(tip.x-mid.x)*t,mid.y+(tip.y-mid.y)*t,mid.z+(tip.z-mid.z)*t};
                float w=.56f*(1-t*.6f);
                g.pipe({p.x-std::sin(a)*w,p.y-.15f,p.z+std::cos(a)*w},{p.x+std::sin(a)*w,p.y-.15f,p.z-std::cos(a)*w},.07f,{76,103,48,255});
            }
        }
        g.props.push_back({{x+.4f,6.1f,z},{.42f,.32f,.42f},{94,81,41,255},-1,0,2});
    };
    // Trunks sit beyond the solid perimeter; only their crowns enter the playable skyline.
    palm(-32.5f,-28);palm(32.5f,-28);palm(-32.5f,29);
    // Low court markings and inset planting beds are flat decoration, without hidden obstacles.
    for(float x:{-24.f,24.f}) {
        g.add(x,.035f,-24,7,.035f,6,{180,126,79,255},1);
        g.add(x,.06f,-26.8f,7,.05f,.12f,pale);
        g.add(x,.06f,-21.2f,7,.05f,.12f,pale);
    }
    // Roof pottery and water tanks break up the skyline without adding invisible ground cover.
    for(auto p:std::array<Vector3,4>{{{-10,7.25f,10},{15,7.85f,13},{-20,6.25f,25},{9,6.85f,-12}}}) {
        g.pipe({p.x,p.y,p.z},{p.x,p.y+.85f,p.z},.65f,{139,124,96,255});
        g.pipe({p.x,p.y+.85f,p.z},{p.x,p.y+.96f,p.z},.7f,pale);
        g.add(p.x+1.4f,p.y+.3f,p.z+.3f,.65f,.6f,.65f,{159,114,77,255},0);
    }
    // A low limewashed cistern dome distinguishes the northern roofline from the flat houses.
    g.props.push_back({{-10,6.55f,-12},{2.4f,1.45f,2.4f},{226,210,181,255},-1,0,2});
    g.pipe({-10,7.95f,-12},{-10,8.45f,-12},.09f,{112,113,90,255});
    // Painted route colors and sector panels are readable from the running height.
    g.label("B","WEST COURT",{-11,2.3f,15.15f},1.45f,1.55f,{145,80,55,255});
    g.label("A","LONG ROAD",{8,2.3f,18.15f},1.45f,1.55f,{145,80,55,255});
    g.label("WEST COURT","SECTOR B",{-15.99f,2.4f,-7.91f},3.7f,1.2f,teal);
    g.label("EAST COURT","SECTOR A",{12,2.4f,-5.91f},4.0f,1.2f,teal);
    g.label("MID GATE","DEPOT DISTRICT",{0,5.35f,9.58f},3.9f,.58f,{90,77,58,255});
    g.label("TF / OLD QUARTER","LONG ROAD",{12,2.5f,18.09f},4.2f,1.0f,teal);
}
