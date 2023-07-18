uniform sampler2D texture;
uniform vec4 tint;
uniform float mix_amount;

float get_hsy_lightness(in vec3 color) {
    return 0.299*color.r + 0.587*color.g + 0.114*color.b;
}

void add_hsy_lightness(inout vec3 color, float light) {
    color.r += light;
    color.g += light;
    color.b += light;
    float l = get_hsy_lightness(color);
    float n = min(color.r, min(color.g, color.b));
    float x = max(color.r, max(color.g, color.b));
    if(n < 0.0) {
        float iln = 1.0 / (l-n);
        color.r = l + ((color.r-l) * l) * iln;
        color.g = l + ((color.g-l) * l) * iln;
        color.b = l + ((color.b-l) * l) * iln;
    }
    if(x > 1.0 && (x-l) > 1e-6) {
        float il  = 1.0 - l;
        float ixl = 1.0 / (x - l);
        color.r = l + ((color.r-l) * il) * ixl;
        color.g = l + ((color.g-l) * il) * ixl;
        color.b = l + ((color.b-l) * il) * ixl;
    }
}

void set_hsy_lightness(inout vec3 color, float light) {
    add_hsy_lightness(color, light - get_hsy_lightness(color));
}

void set_hsy_color(in vec3 src, inout vec3 dest) {
    float lum = get_hsy_lightness(dest);
    dest = src;
    set_hsy_lightness(dest, lum);
}

void main() {
    // lookup the pixel in the texture
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    vec3 tinted_pixel = vec3(pixel);
    set_hsy_color(tint.rgb, tinted_pixel);
    vec3 mixed_rgb_pixel = mix(pixel.rgb, tinted_pixel, mix_amount);
    gl_FragColor = gl_Color * vec4(mixed_rgb_pixel, pixel.a);
}