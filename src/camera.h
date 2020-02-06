#ifndef CS248_CAMERA_H
#define CS248_CAMERA_H

#include <iostream>

#include "collada/camera_info.h"
#include "CS248/matrix3x3.h"

#include "math.h"

namespace CS248 {

/**
 * Camera.
 */
class Camera {
 public:
    /*
      Sets the field of view to match screen screenW/H.
      NOTE: data and screenW/H will almost certainly disagree about the aspect
            ratio. screenW/H are treated as the source of truth, and the field
            of view is expanded along whichever dimension is too narrow.
      NOTE2: info.hFov and info.vFov are expected to be in DEGREES.
    */
    void configure(const Collada::CameraInfo& info, size_t screenW, size_t screenH);

    /*
      Phi and theta are in RADIANS.
    */
    void place(const Vector3D& targetPos, const double phi, const double theta,
               const double r, const double minR, const double maxR);

    /*
      Copies just placement data from the other camera.
    */
    void copyPlacement(const Camera& other);

    /*
      Updates the screen size to be the specified size, keeping screenDist
      constant.
    */
    void setScreenSize(const size_t screenW, const size_t screenH);

    /*
      Translates the camera such that a value at distance d directly in front of
      the camera moves by (dx, dy). Note that dx and dy are in screen coordinates,
      while d is in world-space coordinates (like pos/dir/up).
    */
    void moveBy(const double dx, const double dy, const double d);

    /*
      Move the specified amount along the view axis.
    */
    void moveForward(const double dist);

    /*
      Rotate by the specified amount around the target.
    */
    void rotateBy(const double dPhi, const double dTheta);

    Vector3D getPosition() const { return pos; }
    Vector3D getViewPoint() const { return targetPos; }
    Vector3D getUpDir() const { return c2w[1]; }
    double   getVFov() const { return vFov; }
    double   getAspectRatio() const { return ar; }
    double   getNearClip() const { return nClip; }
    double   getFarClip() const { return fClip; }

 private:
    // Computes pos, screenXDir, screenYDir from target, r, phi, theta.
    void computePosition();

    // Field of view aspect ratio, clipping planes.
    double hFov, vFov, ar, nClip, fClip;

    // Current position and target point (the point the camera is looking at).
    Vector3D pos, targetPos;

    // Orientation relative to target, and min & max distance from the target.
    double phi, theta, r, minR, maxR;

    // camera-to-world rotation matrix (note: also need to translate a
    // camera-space point by 'pos' to perform a full camera-to-world
    // transform)
    Matrix3x3 c2w;

    // Info about screen to render to; it corresponds to the camera's full field
    // of view at some distance.
    size_t screenW, screenH;
    double screenDist;
};

}  // namespace CS248

#endif  // CS248_CAMERA_H
