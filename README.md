# Tangent_Plane_3D
A Calc 3 OpenGL visualizer for tangent planes on 3D surfaces.

## Compile & Run

### Linux
```bash
sudo apt-get install g++ freeglut3-dev libglu1-mesa-dev mesa-common-dev
g++ tangentplane.cpp -o tangent_plane -lGL -lGLU -lglut -lm
./tangent_plane
```

### macOS
```bash
g++ tangentplane.cpp -o tangent_plane -framework OpenGL -framework GLUT -lm
./tangent_plane
```

### Windows (WSL)
1. Install WSL and Ubuntu from the Microsoft Store
2. Inside WSL run:
```bash
sudo apt-get install g++ freeglut3-dev libglu1-mesa-dev mesa-common-dev
g++ tangentplane.cpp -o tangent_plane -lGL -lGLU -lglut -lm
./tangent_plane
```
> Windows 11 supports GUI apps via WSLg automatically.
> Windows 10 users need to install VcXsrv and run `export DISPLAY=:0` before running.

## Controls
- **Arrow keys** — move tangent point
- **Mouse drag** — rotate view
- **Scroll** — zoom
- **T** — toggle tangent plane
- **N** — toggle normal vector
- **R** — reset camera
- **Q / Esc** — quit
