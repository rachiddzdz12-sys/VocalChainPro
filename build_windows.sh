# 1. Créer le dossier de build et s'y déplacer
mkdir build
cd build

# 2. Générer la solution Visual Studio 2022 (Win64 par défaut)
cmake -G "Visual Studio 17 2022" -A x64 ..

# 3. Compiler le projet en mode Release
cmake --build . --config Release
