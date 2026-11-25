#include "film3d.h"
#include "render.h"
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>

namespace film3d {
namespace {

constexpr float PI = 3.14159265f;
struct V3 { float x, y, z; };
V3 sub(V3 a,V3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V3 cross(V3 a,V3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
V3 unit(V3 a){float n=std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);return n>1e-5f?V3{a.x/n,a.y/n,a.z/n}:V3{0,1,0};}
float hash(int n){unsigned x=(unsigned)n*2654435761u;x^=x>>15;x*=2246822519u;x^=x>>13;return float(x&65535u)/65535.0f;}
void color(float r,float g,float b){glColor3f(r,g,b);}
void box(float x,float y,float z,float w,float h,float d){
    glPushMatrix();glTranslatef(x,y,z);glScalef(w,h,d);glBegin(GL_QUADS);
    glNormal3f(0,0,1);glVertex3f(-.5f,-.5f,.5f);glVertex3f(.5f,-.5f,.5f);glVertex3f(.5f,.5f,.5f);glVertex3f(-.5f,.5f,.5f);
    glNormal3f(0,0,-1);glVertex3f(.5f,-.5f,-.5f);glVertex3f(-.5f,-.5f,-.5f);glVertex3f(-.5f,.5f,-.5f);glVertex3f(.5f,.5f,-.5f);
    glNormal3f(1,0,0);glVertex3f(.5f,-.5f,.5f);glVertex3f(.5f,-.5f,-.5f);glVertex3f(.5f,.5f,-.5f);glVertex3f(.5f,.5f,.5f);
    glNormal3f(-1,0,0);glVertex3f(-.5f,-.5f,-.5f);glVertex3f(-.5f,-.5f,.5f);glVertex3f(-.5f,.5f,.5f);glVertex3f(-.5f,.5f,-.5f);
    glNormal3f(0,1,0);glVertex3f(-.5f,.5f,.5f);glVertex3f(.5f,.5f,.5f);glVertex3f(.5f,.5f,-.5f);glVertex3f(-.5f,.5f,-.5f);
    glNormal3f(0,-1,0);glVertex3f(-.5f,-.5f,-.5f);glVertex3f(.5f,-.5f,-.5f);glVertex3f(.5f,-.5f,.5f);glVertex3f(-.5f,-.5f,.5f);glEnd();glPopMatrix();
}
void ellipsoid(float x,float y,float z,float rx,float ry,float rz){
    glPushMatrix();glTranslatef(x,y,z);glScalef(rx,ry,rz);
    for(int j=0;j<10;j++){float v0=-PI/2+j*PI/10,v1=v0+PI/10;glBegin(GL_QUAD_STRIP);for(int i=0;i<=16;i++){float u=i*2*PI/16;for(float v:{v0,v1}){float nx=std::cos(v)*std::cos(u),ny=std::sin(v),nz=std::cos(v)*std::sin(u);glNormal3f(nx,ny,nz);glVertex3f(nx,ny,nz);}}glEnd();}glPopMatrix();
}
void beam(V3 a,V3 b,float w){V3 d=sub(b,a);float len=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);glPushMatrix();glTranslatef(a.x,a.y,a.z);float ang=std::acos(std::max(-1.f,std::min(1.f,d.y/std::max(.001f,len))))*180/PI;if(std::fabs(d.x)+std::fabs(d.z)>.001f)glRotatef(ang,d.z,0,-d.x);else if(d.y<0)glRotatef(180,1,0,0);box(0,len*.5f,0,w,len,w);glPopMatrix();}
void plane(float x,float y,float z,float w,float d,float r,float g,float b){color(r,g,b);glNormal3f(0,1,0);glBegin(GL_QUADS);glVertex3f(x-w/2,y,z-d/2);glVertex3f(x+w/2,y,z-d/2);glVertex3f(x+w/2,y,z+d/2);glVertex3f(x-w/2,y,z+d/2);glEnd();}
void shadow(float x,float z,float sx,float sz){
    glDisable(GL_LIGHTING);glDepthMask(GL_FALSE);glPushMatrix();glTranslatef(x,.025f,z);glRotatef(-90,1,0,0);
    for(int layer=3;layer>=1;--layer){float scale=1.0f+layer*.20f;glColor4f(.015f,.02f,.025f,.055f);glPushMatrix();glScalef(sx*scale,sz*scale,1);glBegin(GL_TRIANGLE_FAN);glVertex3f(0,0,0);for(int i=0;i<=24;i++){float a=i*2*PI/24;glVertex3f(std::cos(a),std::sin(a),0);}glEnd();glPopMatrix();}
    glPopMatrix();glDepthMask(GL_TRUE);glEnable(GL_LIGHTING);
}

void flag(float x,float y,float z,float t){
    glDisable(GL_LIGHTING);glBegin(GL_QUADS);const float bands[]={0,.27f,.32f,.68f,.73f,1};
    const float rgb[5][3]={{.02f,.02f,.025f},{.95f,.93f,.88f},{.72f,.04f,.055f},{.95f,.93f,.88f},{.02f,.38f,.16f}};
    for(int i=0;i<5;i++){glColor3f(rgb[i][0],rgb[i][1],rgb[i][2]);float y0=y-bands[i]*.8f,y1=y-bands[i+1]*.8f;glVertex3f(x,y0,z);glVertex3f(x+1.35f,y0,z+.10f*std::sin(t*2));glVertex3f(x+1.35f,y1,z+.10f*std::sin(t*2+1));glVertex3f(x,y1,z);}glEnd();glEnable(GL_LIGHTING);
}
void poleFlag(float x,float z,float t,bool halfMast=false){color(.65f,.66f,.64f);beam({x,0,z},{x,6.4f,z},.045f);flag(x+.04f,halfMast?4.8f:6.3f,z,t);}

void person(float x,float z,float scale,int id,float t,const Mood&m,bool moving=false,float heading=0.0f){
    float phase=t*(2.0f+0.7f*hash(id+37))+id*1.73f;
    float gait=std::sin(phase),step=std::cos(phase);
    float bob=std::fabs(gait)*.035f*m.energy;
    float skin[4][3]={{.48f,.29f,.17f},{.68f,.43f,.27f},{.78f,.56f,.38f},{.34f,.20f,.12f}};
    float shirts[8][3]={{.15f,.23f,.40f},{.50f,.12f,.13f},{.16f,.34f,.24f},{.32f,.32f,.34f},{.62f,.43f,.16f},{.28f,.18f,.38f},{.72f,.71f,.68f},{.10f,.10f,.12f}};
    int si=(id*3+m.seed)%4,ci=(id*5+m.seed)%8;if(m.mourning)ci=(id%3)?7:6;
    float sy=m.mourning?.65f:shirts[ci][1];
    bool walking=moving||(m.placards&&m.energy>.4f);
    float stride=walking?step*.18f:0.0f;
    float armSwing=walking?gait*.38f:.08f*std::sin(t*1.3f+id*1.91f);
    float idleSway=walking?gait*2.5f:2.0f*std::sin(t*.42f+id*1.37f);
    glPushMatrix();glTranslatef(x,0,z);glScalef(scale,scale,scale);glRotatef(heading+idleSway,0,1,0);shadow(0,0,.46f,.27f);
    color(.08f,.09f,.11f);
    for(float side:{-1.f,1.f}){
        float offset=side<0?stride:-stride;
        float lift=walking?std::max(0.0f,step*(side<0?1.0f:-1.0f))*.12f:0.0f;
        V3 hip={side*.13f,.88f+bob,0},knee={side*.13f+offset*.48f,.48f+bob+lift*.25f,0},ankle={side*.13f+offset,.12f+bob+lift,.02f};
        beam(hip,knee,.15f);beam(knee,ankle,.12f);
        color(.025f,.027f,.03f);box(ankle.x+side*.07f,.055f+lift, .08f,.28f,.12f,.38f);
    }
    color(shirts[ci][0],sy,shirts[ci][2]);box(armSwing*.035f,1.18f+bob,0,.55f,.72f,.31f);
    color(skin[si][0],skin[si][1],skin[si][2]);box(0,1.62f+bob,0,.17f,.16f,.17f);ellipsoid(0,1.86f+bob,0,.20f,.24f,.20f);
    color(.055f,.045f,.04f);ellipsoid(0,2.02f+bob,-.015f,.205f,.095f,.205f);
    for(float side:{-1.f,1.f}){
        bool raised=m.fists&&side>0&&hash(id+71)<.34f;
        float swing=side>0?-armSwing:armSwing;
        float targetY=raised?2.35f:(m.placards&&id%3==0&&side>0?2.15f:1.05f+swing*.28f);
        float targetX=raised?side*.48f:side*.48f;
        V3 sh={side*.31f,1.46f+bob,0},el={side*.42f,raised?1.88f:1.05f+swing*.12f,.02f},hand={targetX,targetY+bob,.04f};
        color(shirts[ci][0],sy,shirts[ci][2]);beam(sh,el,.13f);color(skin[si][0],skin[si][1],skin[si][2]);beam(el,hand,.105f);
    }
    if(m.placards&&id%3==0){color(.48f,.31f,.16f);beam({.44f,1.12f,.08f},{.48f,2.45f,.08f},.035f);color(id%2?.9f:.94f,id%2?.84f:.76f,.64f);box(.50f,2.48f,.08f,.92f,.58f,.08f);glDisable(GL_LIGHTING);color(.08f,.08f,.09f);for(int k=0;k<3;k++)box(.50f,2.60f-k*.12f,.13f,.61f,.025f,.012f);glEnable(GL_LIGHTING);}
    if(m.candles&&id%3==0){glDisable(GL_LIGHTING);glColor4f(1.0f,.58f,.16f,.18f);ellipsoid(.18f,1.38f,.10f,.20f,.30f,.12f);color(1.0f,.78f,.34f);beam({.18f,1.05f,.10f},{.18f,1.27f,.10f},.055f);color(1.0f,.94f,.72f);ellipsoid(.18f,1.34f,.10f,.035f,.09f,.035f);glEnable(GL_LIGHTING);}
    glPopMatrix();
}

void city(float t,int seed){
    color(.11f,.16f,.20f);plane(0,0,-18,100,48,.1f,.1f,.1f);
    for(int i=0;i<18;i++){float x=(i%9-4)*4.6f,z=-15-(i/9)*5.5f,h=4+hash(seed+i)*9;float r=.22f+.16f*hash(seed+30+i),g=.27f+.17f*hash(seed+50+i),b=.32f+.18f*hash(seed+70+i);color(r,g,b);box(x,h/2,z,3.5f,h,3.6f);color(.75f,.64f,.36f);glDisable(GL_LIGHTING);for(int w=0;w<3;w++)for(int q=0;q<int(h/2);q++)box(x-1+w,q*1.1f+.9f,z+1.82f,.28f,.36f,.025f);glEnable(GL_LIGHTING);}
    // Distant urban haze and street furniture establish a Nairobi street scale.
    color(.20f,.23f,.24f);box(0,.16f,0,26,.24f,42);color(.12f,.13f,.14f);box(0,.12f,3,11,.16f,35);
    glDisable(GL_LIGHTING);color(.78f,.74f,.59f);for(int i=-4;i<=4;i++)box(i*1.55f,.22f,5, .09f,.025f,1.15f);glEnable(GL_LIGHTING);
    float bx=std::fmod(t*2.2f+seed*2.4f,42.f)-21.f;color(.16f,.42f,.30f);box(bx,.75f,1,3.1f,1.35f,5.0f);color(.14f,.21f,.26f);box(bx,.95f,3.55f,2.55f,.62f,.025f);color(.06f,.07f,.075f);for(float w:{-1.f,1.f})for(float z:{-.32f,.32f})ellipsoid(bx+w*1.0f,.37f,1+z*3,.38f,.38f,.38f);
    for(int i=-2;i<=2;i++){
        float lx=i*5.0f; color(.12f,.13f,.14f);beam({lx,.1f,7},{lx,5.8f,7},.12f);beam({lx,5.6f,7},{lx+.8f,5.6f,7},.10f);
        glDisable(GL_LIGHTING);glDepthMask(GL_FALSE);glColor4f(1.0f,.68f,.28f,.12f);ellipsoid(lx+.85f,5.5f,7,.58f,.5f,.42f);color(1.0f,.76f,.42f);ellipsoid(lx+.85f,5.5f,7,.20f,.16f,.17f);glDepthMask(GL_TRUE);glEnable(GL_LIGHTING);
    }
}
void civicBuilding(Loc loc,float t,const Mood&m){
    color(.64f,.62f,.56f);box(0,1.9f,-7,12,3.8f,5.2f);color(.78f,.76f,.68f);box(0,4.05f,-7,13,.35f,6.2f);
    if(loc==Loc::BOMAS){color(.56f,.44f,.35f);ellipsoid(0,4.2f,-7,5.8f,2.0f,3.1f);color(.16f,.12f,.10f);box(0,1.2f,-4.34f,1.8f,2.4f,.12f);}
    else{
        color(.70f,.68f,.61f);box(0,5.05f,-7,12,.45f,5.5f);
        for(int i=-3;i<=3;i++){color(.78f,.76f,.69f);box(i*1.55f,3.0f,-3.95f,.55f,3.4f,.65f);color(.53f,.51f,.46f);box(i*1.55f,4.8f,-3.95f,.85f,.20f,.85f);}
        color(.70f,.68f,.61f);glBegin(GL_TRIANGLES);glNormal3f(0,0,1);glVertex3f(-6,5.2f,-3.8f);glVertex3f(6,5.2f,-3.8f);glVertex3f(0,7.5f,-3.8f);glEnd();
        color(.20f,.15f,.12f);box(0,1.15f,-3.75f,1.5f,2.3f,.15f);
    }
    poleFlag(-8,-6,t,m.halfMast);poleFlag(8,-6,t,m.halfMast);
}
void ballotSet(float t,const Mood&m){
    color(.72f,.66f,.54f);box(0,2,-8,13,4,6);color(.55f,.25f,.17f);box(0,4.15f,-8,13,.35f,6.2f);
    for(int i=-2;i<=2;i++){color(.22f,.31f,.37f);box(i*2.05f,2.15f,-4.92f,1.35f,2.3f,.12f);color(.09f,.14f,.17f);box(i*2.05f,2.15f,-4.82f,1.13f,1.95f,.06f);}
    color(.42f,.28f,.17f);box(5,1.02f,-2.7f,2.5f,.2f,1.1f);color(.86f,.84f,.75f);box(4.5f,1.16f,-3.0f,.55f,.04f,.7f);
    color(.06f,.07f,.10f);box(0,.78f,-1.8f,.82f,1.55f,.82f);color(.12f,.25f,.46f);box(0,.82f,-1.8f,1.0f,1.45f,1.0f);color(.035f,.04f,.05f);box(0,1.57f,-1.8f,.5f,.06f,.35f);
    float cyc=std::fmod(t+1.4f,5.f);if(cyc<2){color(.96f,.94f,.86f);box(0,1.72f+(cyc/2.f)*.48f,-1.8f,.42f,.035f,.3f);}
    poleFlag(-8,0,t,m.halfMast);
}
void stadium(float t){color(.31f,.33f,.36f);for(int row=0;row<5;row++){box(0,1.0f+row*.65f,-9+row*.72f,25,1.25f,2.0f);for(int i=0;i<58;i++){float x=-11.5f+i*.4f,z=-9+row*.72f;int c=(i*7+row*11)%6;color(c==0?.65f:.18f,c==1?.42f:.28f,c==2?.2f:.34f);ellipsoid(x,1.9f+row*.65f,z,.12f,.17f,.12f);}}color(.20f,.40f,.23f);plane(0,.02f,2,19,15,.2f,.4f,.2f);}
void homestead(){color(.08f,.35f,.48f);plane(0,.01f,-8,32,16,.08f,.35f,.5f);color(.34f,.43f,.27f);for(int i=0;i<5;i++){float x=-11+i*5.5f;color(.25f,.22f,.13f);box(x,2.0f,-5,1.3f,4,1.3f);color(.12f,.34f,.18f);ellipsoid(x,4.0f,-5,2.0f,1.25f,1.5f);}color(.56f,.42f,.30f);box(5,1.5f,-3,4.5f,3,3.5f);color(.35f,.18f,.12f);glBegin(GL_TRIANGLES);glNormal3f(0,0,1);glVertex3f(2.5f,3,-1.25f);glVertex3f(7.5f,3,-1.25f);glVertex3f(5,4.7f,-1.25f);glEnd();}

void lookAt(V3 eye,V3 target){V3 f=unit(sub(target,eye)),s=unit(cross(f,{0,1,0})),u=cross(s,f);GLfloat m[]={s.x,u.x,-f.x,0,s.y,u.y,-f.y,0,s.z,u.z,-f.z,0,0,0,0,1};glMultMatrixf(m);glTranslatef(-eye.x,-eye.y,-eye.z);}
float ease(float a,float b,float v){float x=std::max(0.0f,std::min(1.0f,(v-a)/(b-a)));return x*x*(3-2*x);}
V3 mix(V3 a,V3 b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};}
void setupCamera(float t,int seed,Loc loc){
    float orbit=std::sin(t*.12f+seed)*.045f;
    V3 eye={orbit*18.f,6.7f,24.f},target={0,2.6f,-1.f};
    if(loc==Loc::BALLOT){
        float track=ease(2.5f,7.5f,t),overview=ease(9.0f,12.0f,t);
        V3 trackingEye={-5.5f+11.0f*track,4.8f,18.5f},trackingTarget={0,1.8f,0};
        V3 highEye={0,13.0f,17.0f},highTarget={0,0.8f,2.0f};
        eye=mix(mix({-1.8f+orbit*8,5.5f,20.f},trackingEye,track),highEye,overview);
        target=mix(mix({0,2.1f,-1.f},trackingTarget,track),highTarget,overview);
    }else if(loc==Loc::STREET){
        float track=ease(2.0f,6.0f,t),overview=ease(9.0f,12.0f,t);
        V3 trackingEye={-7.0f+14.0f*track,5.3f,20.0f},trackingTarget={0,1.4f,0};
        V3 highEye={0,14.0f,18.0f},highTarget={0,0.5f,0};
        eye=mix(mix(eye,trackingEye,track),highEye,overview);
        target=mix(mix(target,trackingTarget,track),highTarget,overview);
    }else if(loc==Loc::STADIUM){eye={0,9.0f,29.f};target={0,3.4f,-2.f};}
    eye.x+=std::sin(t*1.7f+seed)*.025f;
    lookAt(eye,target);
}
void scene(Loc loc,float t,const Mood&m,const simulation::Simulation* sim){
    // Softly graded sky plane, distant city, and a broad textured-looking ground.
    float skyTop[3]={.18f,.30f,.46f},skyEdge[3]={.68f,.70f,.64f};
    if(m.sky==0){skyTop[0]=.12f;skyTop[1]=.18f;skyTop[2]=.38f;skyEdge[0]=.82f;skyEdge[1]=.50f;skyEdge[2]=.34f;}
    if(m.sky==2){skyTop[0]=.20f;skyTop[1]=.45f;skyTop[2]=.78f;skyEdge[0]=.83f;skyEdge[1]=.82f;skyEdge[2]=.71f;}
    if(m.sky==3){skyTop[0]=.08f;skyTop[1]=.10f;skyTop[2]=.27f;skyEdge[0]=.76f;skyEdge[1]=.34f;skyEdge[2]=.22f;}
    if(m.sky==4){skyTop[0]=.012f;skyTop[1]=.018f;skyTop[2]=.06f;skyEdge[0]=.12f;skyEdge[1]=.14f;skyEdge[2]=.20f;}
    if(m.sky==5){skyTop[0]=.045f;skyTop[1]=.07f;skyTop[2]=.18f;skyEdge[0]=.34f;skyEdge[1]=.27f;skyEdge[2]=.32f;}
    glDisable(GL_LIGHTING);glBegin(GL_QUADS);glColor3f(skyTop[0],skyTop[1],skyTop[2]);glVertex3f(-90,45,-65);glVertex3f(90,45,-65);glColor3f(skyEdge[0],skyEdge[1],skyEdge[2]);glVertex3f(90,0,-65);glVertex3f(-90,0,-65);glEnd();glEnable(GL_LIGHTING);
    plane(0,0,0,100,82,.35f,.34f,.30f);
    if(loc==Loc::STREET){city(t,m.seed);}
    else if(loc==Loc::BALLOT){ballotSet(t,m);}
    else if(loc==Loc::STADIUM){stadium(t);}
    else if(loc==Loc::HOMESTEAD){homestead();}
    else if(loc==Loc::COURT||loc==Loc::BOMAS||loc==Loc::PARLIAMENT||loc==Loc::STATE_HOUSE){civicBuilding(loc,t,m);}
    if(loc==Loc::PARLIAMENT||loc==Loc::STATE_HOUSE){city(t,m.seed+4);}
    if(sim){
        for(const simulation::Barrier& barrier:sim->barriers()){
            color(.34f,.36f,.34f);box(barrier.x,.36f,barrier.y,barrier.radius*2.4f,.72f,.5f);
            color(.70f,.68f,.57f);for(float stripe=-.5f;stripe<=.5f;stripe+=1.0f)box(barrier.x+stripe*barrier.radius,.38f,barrier.y+.27f,.16f,.48f,.035f);
        }
    }
    if(sim){
        for(const simulation::Agent&a:sim->agents()){
            if(a.state==simulation::State::Finished||a.state==simulation::State::GaveUp)continue;
            Mood agentMood=m;
            if(sim->mode()==simulation::Mode::Protest)agentMood.energy=std::max(agentMood.energy,.48f);
            bool moving=a.state==simulation::State::Approaching||a.state==simulation::State::Queuing||a.state==simulation::State::ToBallotBox||a.state==simulation::State::Leaving||a.state==simulation::State::Gathering||a.state==simulation::State::Dispersing;
            float x=a.position.x,z=a.position.y+(sim->mode()==simulation::Mode::Protest?-2.0f:0.0f);
            float ageScale=a.age==simulation::AgeGroup::Youth?.96f:(a.age==simulation::AgeGroup::Senior?.93f:1.0f);
            float scale=(.88f+(hash(static_cast<int>(a.id)*31+m.seed)-.5f)*.12f)*ageScale;
            person(x,z,scale,static_cast<int>(a.id),t,agentMood,moving,a.heading);
        }
    } else if(loc!=Loc::BALLOT&&loc!=Loc::STADIUM){int n=std::max(0,std::min(m.crowd,86)),cols=(n+3)/4;float spacing=cols>1?std::min(1.0f,18.0f/(cols-1)):1.0f;for(int i=0;i<n;i++){float row=float(i%4),x=(float(i/4)-float(cols-1)*.5f)*spacing+(hash(i*17+m.seed)-.5f)*.32f,z=2+row*2.1f+hash(i*7+m.seed)*.5f;float size=.95f+(hash(i*29+m.seed)-.5f)*.12f;person(x,z,size,i,t,m);}}
    if(!sim&&loc==Loc::BALLOT){for(int i=0;i<15;i++){float x=-3.4f+(i%2)*.78f+(hash(i+91)-.5f)*.12f;float z=1.5f+(i/2)*.78f;person(x,z,.9f+(hash(i+51)-.5f)*.08f,i,t,m);}}
    if(loc==Loc::STADIUM){for(int i=0;i<22;i++)person((i%11-5)*.95f,4+(i/11)*1.3f,.9f,i,t,m);}
}
}

void drawLocation(Loc loc,float time,const Mood&m,const simulation::Simulation* sim){
    glPushAttrib(GL_ALL_ATTRIB_BITS);glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();float nearPlane=.1f,top=nearPlane*std::tan(42.f*PI/360.f),aspect=float(render::width())/std::max(1,render::height());glFrustum(-top*aspect,top*aspect,-top,top,nearPlane,140.f);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glEnable(GL_NORMALIZE);glEnable(GL_COLOR_MATERIAL);glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);glEnable(GL_LIGHT1);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDisable(GL_CULL_FACE);
    const float night=m.sky>=4?.34f:1.0f;GLfloat ambient[]={.42f*night,.43f*night,.46f*night,1},diffuse[]={.92f*night,.88f*night,.78f*night,1},pos[]={-7,14,11,1};glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);glLightfv(GL_LIGHT0,GL_POSITION,pos);GLfloat fill[]={.22f*night,.28f*night,.38f*night,1},p2[]={8,8,-5,1};glLightfv(GL_LIGHT1,GL_DIFFUSE,fill);glLightfv(GL_LIGHT1,GL_POSITION,p2);
    glClearColor(.18f,.25f,.32f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);setupCamera(time,m.seed,loc);
    GLfloat fogColor[]={.42f,.46f,.48f,1};if(m.sky>=4){fogColor[0]=.20f;fogColor[1]=.23f;fogColor[2]=.30f;}glFogi(GL_FOG_MODE,GL_LINEAR);glFogfv(GL_FOG_COLOR,fogColor);glFogf(GL_FOG_START,28.0f);glFogf(GL_FOG_END,95.0f);glEnable(GL_FOG);
    scene(loc,time,m,sim);glDisable(GL_FOG);
    glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glPopAttrib();
}
}
