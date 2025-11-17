#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;

uniform samplerExternalOES u_texture;
uniform vec2 u_textureSize;
uniform float u_focusMode;      // 0.0 = off, 1.0 = on
uniform float u_zebraPattern;   // 0.0 = off, 1.0 = on

in vec2 v_texCoord;
out vec4 fragColor;

float laplacian(samplerExternalOES tex, vec2 coord, vec2 texelSize) {
    float result = 0.0;
    result += -1.0 * texture(tex, coord + vec2(-texelSize.x, 0.0)).r;
    result += -1.0 * texture(tex, coord + vec2(texelSize.x, 0.0)).r;
    result += -1.0 * texture(tex, coord + vec2(0.0, -texelSize.y)).r;
    result += -1.0 * texture(tex, coord + vec2(0.0, texelSize.y)).r;
    result += 4.0 * texture(tex, coord).r;
    return abs(result);
}

void main() {
    vec3 color = texture(u_texture, v_texCoord).rgb;

    if (u_zebraPattern > 0.5) {
        float luminance = dot(color, vec3(0.299, 0.587, 0.114));

        bool overexposed = luminance > 0.96;
        bool underexposed = luminance < 0.04;

        float diag = (v_texCoord.x + v_texCoord.y) * 75.0;
        float stripe = step(0.5, fract(diag));

        if (overexposed) {
            fragColor = vec4(mix(vec3(1.0, 0.0, 0.0), color, stripe), 1.0);
            return;
        }
        if (underexposed) {
            fragColor = vec4(mix(vec3(0.0, 0.0, 1.0), color, stripe), 1.0);
            return;
        }
    }

    if (u_focusMode > 0.5) {
        vec2 texelSize = 1.0 / u_textureSize;
        float sharpness = laplacian(u_texture, v_texCoord, texelSize);
        float normalizedSharpness = clamp(sharpness * 20.0, 0.0, 1.0);
        float isFocused = step(1.0, normalizedSharpness);

        vec3 overlay = mix(color, vec3(0.0, 1.0, 0.0), isFocused * 0.4);
        fragColor = vec4(overlay, 1.0);
    } else {
        fragColor = vec4(color, 1.0);
    }
}