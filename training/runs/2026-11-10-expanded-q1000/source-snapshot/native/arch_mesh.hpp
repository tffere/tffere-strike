#pragma once
// A watertight, continuous voussoir profile, shared by all sandstone arches.
inline Mesh makeArchMesh(){
    std::vector<float> v,n,uv;
    auto triangle=[&](Vector3 a,Vector3 b,Vector3 c){
        Vector3 normal=Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b,a),Vector3Subtract(c,a)));
        for(auto p:{a,b,c}){v.insert(v.end(),{p.x,p.y,p.z});n.insert(n.end(),{normal.x,normal.y,normal.z});uv.insert(uv.end(),{p.x*.4f+.5f,p.y*.4f});}
    };
    auto quad=[&](Vector3 a,Vector3 b,Vector3 c,Vector3 d){triangle(a,b,c);triangle(a,c,d);};
    auto p=[](float a,float r,float z){return Vector3{std::cos(a)*r,std::sin(a)*r,z};};
    constexpr int count=40;
    for(int i=0;i<count;i++){
        float a=i*PI/count,b=(i+1)*PI/count;
        quad(p(a,1,.5f),p(a,1.22f,.5f),p(b,1.22f,.5f),p(b,1,.5f));
        quad(p(a,1,-.5f),p(b,1,-.5f),p(b,1.22f,-.5f),p(a,1.22f,-.5f));
        quad(p(a,1.22f,.5f),p(a,1.22f,-.5f),p(b,1.22f,-.5f),p(b,1.22f,.5f));
        quad(p(a,1,.5f),p(b,1,.5f),p(b,1,-.5f),p(a,1,-.5f));
    }
    quad(p(0,1,.5f),p(0,1,-.5f),p(0,1.22f,-.5f),p(0,1.22f,.5f));
    quad(p(PI,1,.5f),p(PI,1.22f,.5f),p(PI,1.22f,-.5f),p(PI,1,-.5f));
    Mesh m{};m.vertexCount=int(v.size()/3);m.triangleCount=m.vertexCount/3;
    m.vertices=(float*)MemAlloc(v.size()*sizeof(float));m.normals=(float*)MemAlloc(n.size()*sizeof(float));m.texcoords=(float*)MemAlloc(uv.size()*sizeof(float));
    std::copy(v.begin(),v.end(),m.vertices);std::copy(n.begin(),n.end(),m.normals);std::copy(uv.begin(),uv.end(),m.texcoords);UploadMesh(&m,false);return m;
}
