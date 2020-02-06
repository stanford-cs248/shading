#include "camera.h"

#include "CS248/misc.h"
#include "CS248/vector3D.h"

using std::cout;
using std::endl;
using std::max;
using std::min;

namespace CS248 {

using Collada::CameraInfo;

void Camera::configure(const CameraInfo& info, size_t screenW, size_t screenH) {
    this->screenW = screenW;
    this->screenH = screenH;
    nClip = info.nClip;
    fClip = info.fClip;
    hFov = info.hFov;
    vFov = info.vFov;
  //  targetPos = info.view_dir;
  //  acos(c_dir.y), atan2(c_dir.x, c_dir.z)

    double ar1 = tan(radians(hFov) / 2) / tan(radians(vFov) / 2);
    ar = static_cast<double>(screenW) / screenH;
    if (ar1 < ar) {
        // hFov is too small
        hFov = 2 * degrees(atan(tan(radians(vFov) / 2) * ar));
    } else if (ar1 > ar) {
        // vFov is too small
        vFov = 2 * degrees(atan(tan(radians(hFov) / 2) / ar));
    }
    screenDist = ((double)screenH) / (2.0 * tan(radians(vFov) / 2));
}

void Camera::place(const Vector3D& targetPos, const double phi,
                   const double theta, const double r, const double minR,
                   const double maxR) {
    double r_ = min(max(r, minR), maxR);
    double phi_ = (sin(phi) == 0) ? (phi + EPS_F) : phi;
    this->targetPos = targetPos;
    this->phi = phi_;
    this->theta = theta;
    this->r = r_;
    this->minR = minR;
    this->maxR = maxR;
    computePosition();
}

void Camera::copyPlacement(const Camera& other) {
    pos = other.pos;
    targetPos = other.targetPos;
    phi = other.phi;
    theta = other.theta;
    minR = other.minR;
    maxR = other.maxR;
    c2w = other.c2w;
}

void Camera::setScreenSize(const size_t screenW, const size_t screenH) {
    this->screenW = screenW;
    this->screenH = screenH;
    ar = 1.0 * screenW / screenH;
    hFov = 2 * degrees(atan(((double)screenW) / (2 * screenDist)));
    vFov = 2 * degrees(atan(((double)screenH) / (2 * screenDist)));
}

void Camera::moveBy(const double dx, const double dy, const double d) {
    const double scaleFactor = d / screenDist;
    const Vector3D& displacement = c2w[0] * (dx * scaleFactor) + c2w[1] * (dy * scaleFactor);
    pos += displacement;
    targetPos += displacement;
}

void Camera::moveForward(const double dist) {
    double newR = min(max(r - dist, minR), maxR);
    pos = targetPos + ((pos - targetPos) * (newR / r));
    r = newR;
}

void Camera::rotateBy(const double dPhi, const double dTheta) {
    phi = clamp(phi + dPhi, 0.0, (double)PI);
    theta += dTheta;
    computePosition();
}

void Camera::computePosition() {
    double sinPhi = sin(phi);
    if (sinPhi == 0) {
        phi += EPS_F;
        sinPhi = sin(phi);
    }

    const Vector3D dirToCamera(r * sinPhi * sin(theta), r * cos(phi), r * sinPhi * cos(theta));
    pos = targetPos + dirToCamera;
    Vector3D upVec(0, sinPhi > 0 ? 1 : -1, 0);
    Vector3D screenXDir = cross(upVec, dirToCamera);
    screenXDir.normalize();
    Vector3D screenYDir = cross(dirToCamera, screenXDir);
    screenYDir.normalize();

    // the camera's view direction is the opposite of of dirToCamera, so
    // directly using dirToCamera as column 2 of the matrix takes [0 0 -1]
    // to the world space view direction (the camera is looking in -Z)

    c2w[0] = screenXDir;
    c2w[1] = screenYDir;
    c2w[2] = dirToCamera.unit();  
}

}  // namespace CS248
