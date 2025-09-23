#!/bin/bash

# ==========================
# Nautical Arduino Monitor
# Script para subir el proyecto a GitHub
# Autor: Hernzum
# ==========================

# Nombre del repositorio
REPO_NAME="Nautical-Arduino-Monitor"

# Usuario de GitHub (Pídele al usuario que lo ingrese si no está configurado)
if [[ -z "$GITHUB_USER" ]]; then
    read -p "Ingrese su usuario de GitHub: " GITHUB_USER
fi

# Dirección del repositorio en GitHub
GITHUB_REPO="https://github.com/$GITHUB_USER/$REPO_NAME.git"

# Verificar si Git está instalado
if ! command -v git &> /dev/null; then
    echo "❌ Git no está instalado. Por favor instálalo antes de continuar."
    exit 1
fi

# Verificar si el repositorio ya existe en GitHub
echo "🔍 Verificando si el repositorio existe en GitHub..."
if curl -s "https://api.github.com/repos/$GITHUB_USER/$REPO_NAME" | grep -q '"Not Found"'; then
    echo "❌ El repositorio no existe en GitHub. Debes crearlo primero en https://github.com/new"
    exit 1
else
    echo "✅ Repositorio encontrado en GitHub."
fi

# Inicializar Git si no está inicializado
if [ ! -d ".git" ]; then
    echo "🔧 Inicializando Git..."
    git init
    git branch -M main
    git remote add origin "$GITHUB_REPO"
fi

# Agregar archivos al repositorio
echo "📂 Agregando archivos..."
git add .

# Confirmar el commit con un mensaje
echo "📝 Realizando commit..."
git commit -m "Initial commit - Nautical Arduino Monitor"

# Subir archivos a GitHub
echo "🚀 Subiendo archivos a GitHub..."
git push -u origin main

# Verificar si la subida fue exitosa
if [ $? -eq 0 ]; then
    echo "✅ ¡Proyecto subido con éxito a GitHub!"
    echo "🌍 Puedes verlo aquí: https://github.com/$GITHUB_USER/$REPO_NAME"
else
    echo "❌ Error al subir el proyecto. Verifica tu conexión y credenciales."
fi
