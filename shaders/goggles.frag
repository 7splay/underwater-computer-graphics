#version 330 core

// goggle vignette: two soft eye windows joined by a dip-free bridge
// using the sum of two gaussians so brightness stays high at the center

in vec2 vUv;

out vec4 FragColor;

uniform vec2  uResolution;
uniform float uTime;
uniform float uSigma;     // radius of each eye (smaller = sharper)
uniform float uEyeSep;    // distance between eye centers
uniform float uDarkness;  // frame opacity multiplier

void main() {
    float aspect = uResolution.x / uResolution.y;
    vec2 p = vec2((vUv.x - 0.5) * aspect, vUv.y - 0.5);

    vec2 leftCenter  = vec2(-uEyeSep, 0.0);
    vec2 rightCenter = vec2( uEyeSep, 0.0);

    float dL = length(p - leftCenter);
    float dR = length(p - rightCenter);
    float leftEye  = exp(-dL * dL / (2.0 * uSigma * uSigma));
    float rightEye = exp(-dR * dR / (2.0 * uSigma * uSigma));

    float sum = leftEye + rightEye;
    float brightness = pow(clamp(sum, 0.0, 1.5) / 1.5, 1.6);

    float alpha = 1.0 - smoothstep(0.05, 0.30, brightness);

    // darkness > 1 darkens the frame color; <= 1 just scales opacity
    vec3 frameColor = vec3(0.02, 0.04, 0.06) / max(uDarkness, 1.0);
    float alphaMul = clamp(uDarkness, 0.0, 1.0);
    FragColor = vec4(frameColor, alpha * alphaMul);
}
