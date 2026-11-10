#pragma once
// Included inside namespace strike, after the collision types.
inline constexpr float DESERT_LIMIT = 31.5f;
inline constexpr Vec2 DESERT_PLAYER_SPAWN{0,26};
inline constexpr std::array<Vec2,6> DESERT_BOT_SPAWNS{{{-26,-25},{27,-25},{0,-16},{25,4},{-25,2},{0,3}}};
inline std::vector<Obstacle> desertObstacles() {
    std::vector<Obstacle> o;
    auto box=[&](float x,float z,float w,float d,float h,ObstacleKind k=ObstacleKind::Building){o.push_back({x,z,w,d,h,k});};
    box(-32,0,2,66,6,ObstacleKind::Wall); box(32,0,2,66,6,ObstacleKind::Wall);
    box(0,-32,66,2,6,ObstacleKind::Wall); box(0,32,66,2,6,ObstacleKind::Wall);
    box(-12,5,14,20,7.2f); box(-10,-12,10,8,6.5f);
    box(12,7,14,22,7.8f); box(12,-11,14,10,6.8f);
    box(-18,24,14,8,6.2f); box(15,24,14,8,6.5f);
    box(0,-23,2,10,3.4f,ObstacleKind::Wall);
    // Mid doors: the opening remains wide enough for two characters.
    box(-3.6f,9,2.8f,.65f,3.5f,ObstacleKind::Wall); box(3.6f,9,2.8f,.65f,3.5f,ObstacleKind::Wall);
    // Tunnel arcade pillars stay against its edges, leaving an uninterrupted route.
    for(float z : {14.f,7.f,0.f,-7.f}) {box(-29.3f,z,1.2f,1.2f,3.8f,ObstacleKind::Wall);box(-20.7f,z,1.2f,1.2f,3.8f,ObstacleKind::Wall);}
    // Courtyard cover, staggered rather than sealing any route.
    box(-23,-24,3.8f,3.2f,1.55f,ObstacleKind::Crate); box(-20,-20,2,2,2.5f,ObstacleKind::Crate);
    box(24,-24,4.2f,3.5f,1.5f,ObstacleKind::Crate); box(20,-20,2.2f,2.2f,2.8f,ObstacleKind::Crate);
    box(-1,-11,2.3f,2.3f,1.4f,ObstacleKind::Crate);
    box(26,12,2.4f,3.8f,1.25f,ObstacleKind::Crate);
    box(-27,21,2.3f,2.3f,1.7f,ObstacleKind::Crate);
    for(auto p:std::array<Vec2,8>{{{-28,-18},{-27,-18},{28,-12},{28,-11},{-7,17},{-8,17},{23,29},{-28,28}}}) box(p.x,p.z,.85f,.85f,1.25f,ObstacleKind::Barrel);
    return o;
}
