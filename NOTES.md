
# 04/01/2021 - Intro setup

IBL : Image Based Lighting

Ce projet sert à manipuler une API graphique (ici OpenGL) pour produires différents effets.

Expérimentation de live coding (on part de zéro).

- Prérequis
  - Visual Studio 2019
  - (Vérifier que tout le monde arrive à utiliser OpenGL 3.3 Core Profile car Intel HD Graphics < 3000 ne va que jusqu'à la 3.1)

Préparation du projet :
 - GLFW/glad/ImGui
 - Autres dépendances : stb_image, stb_perlin, tinyobjloader

- Récap OpenGL :
  - Buffers
  - Vertex layout
  - Vertex shader
  - Pixel shader

- Nouveaux concepts
  - Extensions (démonstration avec EXT_texture_filter_anisotropic)
  - FBO (pour post-process)

Fonctionnalités communes :
 - Algèbre linéaire nécessaire à la 3D (float3, float4, mat4x4)
 - Camera
 - Générateur de mesh
 - Logs d'erreur OpenGL (KHR_DEBUG)
 - Debugger visuel

Divers :
 - Comment utiliser Renderdoc (analyse de frame (Everspace? Northgard))
 - Comment inspecter l'état OpenGL (ex: modifier un shader en temps réel !)

- Effets :
  - Skybox
  - Scrolling textures (Mario Galaxy 2 : https://www.youtube.com/watch?v=8rCRsOLiO7k)
  - Billboards :
    - VFX :
      - League Of Legends : https://www.youtube.com/watch?v=I6S-NEF-UlM
      - Diablo 2 : https://www.youtube.com/watch?v=YPy2hytwDLM
    - Impostors
      - https://forum.unity.com/threads/released-imposter-system-optimization.426478/
      - https://www.youtube.com/watch?v=JOL5e-J1btA
  - Post process
    - Color correction
    - Bloom

Autres :
 - https://www.youtube.com/watch?v=4B00hV3wmMY
 - http://www.adriancourreges.com/blog/2016/09/09/doom-2016-graphics-study/
 - https://twitter.com/NoelFB/status/1341287223040217088

1. Setup
00:00:00 Intro
00:19:15 Compilation du projet exemple d'ImGui glfw/gl3.3
00:25:56 Création du projet ibl
00:41:17 main() avec glfw
00:52:44 glad
01:07:44 Extension KHR_DEBUG
01:18:00 Intégration ImGui

2. Setup
00:00:00 ImGui
00:21:05 Désactivation de certains logs KHR_DEBUG
00:23:30 ImGui suite
00:29:00 Classe Demo
00:41:33 DemoTriangle

3. Setup
00:00:00 DemoTriangle suite
00:23:37 Le triangle s'affiche
00:26:57 Unions
00:42:30 Matrices
00:49:30 Uniforms
01:04:26 Ajout d'un quad
01:06:20 Texture
01:22:59 stb_perlin
01:40:39 Renderdoc
01:59:51 Extension OpenGL filtre anisotropique

# 05/01/2021 - FBO

4. FBO
00:00:00 Création FBO

# 06/01/2021
00:00:00 Multitexturing + Multiple color outputs (fbo)
00:42:00 
00:01:13 Intégration de la caméra à DemoQuad

- À vous de jouer
  - Implémenter un effet :
    - VFX diablo 2 : https://www.youtube.com/watch?v=YPy2hytwDLM
    - Bloom learnopengl
    - Scrolling texture

- Ne pas modifier les sources

git remote add origin ssh://git@git.isartintra.com:2424/2020/GP_2023_IBL/{nom_du_groupe}.git