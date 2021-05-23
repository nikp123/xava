precision mediump float;

// color determined by the vertex shader
varying vec4 v_color;

void main() {
  gl_FragColor = v_color;
}

