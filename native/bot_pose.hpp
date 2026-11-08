#pragma once
#include <algorithm>
#include <cmath>

namespace strike {
inline float poseBlend(float t,float start,float end) {
    float u=std::clamp((t-start)/(end-start),0.f,1.f);
    return u*u*(3-2*u);
}
struct CollapsePose {
    float hipHeight, legFold, torsoPitch, torsoRoll, armRelease, settle;
};
inline CollapsePose collapsePose(float progress,float standingHip,float direction,int variant) {
    float buckle=poseBlend(progress,0,.42f),fall=poseBlend(progress,.18f,.86f);
    float settle=poseBlend(progress,.75f,1.f);
    // Knee failure precedes torso impact; the torso rotates around the pelvis,
    // independently of planted feet. A small damped rebound follows impact.
    float hip=standingHip*(1-fall)+.24f*fall-.25f*buckle*(1-fall);
    float rebound=std::sin(settle*3.14159265f)*.025f;
    return {std::max(.22f,hip)+rebound,poseBlend(progress,.10f,.9f),
            direction*(87.f*fall-3.f*std::sin(settle*3.14159265f)),
            (variant%2?1.f:-1.f)*7.f*fall,
            poseBlend(progress,.02f,.65f),settle};
}
}
