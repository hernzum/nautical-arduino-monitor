#!/bin/bash
# ===================================================
# ✅ Script: Verificar y Subir a GitHub
# Versión: 1.0
# Creador: @hernzum
# Repo: https://github.com/hernzum/nautical-arduino-monitor
# 
# Funciones:
#   - Verifica existencia del repositorio
#   - Valida el token PAT
#   - Comprueba permisos de escritura
#   - Corrige rama master → main
#   - Hace commit y push si hay cambios
# ===================================================

set -euo pipefail

# ======================== CONFIGURACIÓN ========================
REPO_OWNER="hernzum"
REPO_NAME="nautical-arduino-monitor"
GITHUB_TOKEN="${GITHUB_TOKEN:-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}"
PROJECT_DIR="/home/nito/nautical-arduino-monitor"  # Cambia si es diferente
TARGET_BRANCH="main"
LOG_FILE="$PROJECT_DIR/.github-check.log"

# URLs API
REPO_URL="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME"
AUTH_TEST_URL="https://api.github.com/user"
REMOTE_URL="https://$GITHUB_TOKEN@github.com/$REPO_OWNER/$REPO_NAME.git"

log() {
  echo "[$(date '+%H:%M:%S')] $1"
}

error_exit() {
  log "❌ ERROR: $1"
  echo "" >> "$LOG_FILE"
  exit 1
}

safe_run() {
  "$@" || error_exit "Comando fallido: $*"
}

# ======================== INICIO DEL REGISTRO ========================
{
  echo
  log "🔍 Iniciando verificación completa para GitHub"
  log "Proyecto: $PROJECT_DIR"
  log "Usuario: $REPO_OWNER"
  echo

  # ======================== VERIFICAR GIT INSTALADO ========================
  if ! command -v git &> /dev/null; then
    error_exit "Git no está instalado. Usa: sudo apt install git"
  fi

  cd "$PROJECT_DIR" || error_exit "No se pudo acceder a $PROJECT_DIR"

  # ======================== VERIFICAR TOKEN NO VACÍO ========================
  if [ -z "$GITHUB_TOKEN" ] || [ "$GITHUB_TOKEN" = "github_pat_..." ]; then
    error_exit "Token no configurado. Define GITHUB_TOKEN o edita el script."
  fi

  # ======================== VERIFICAR CONEXIÓN A INTERNET ========================
  log "🌐 Verificando conexión a internet..."
  if ! curl -fsS --head https://github.com > /dev/null; then
    error_exit "Sin conexión a internet"
  fi
  log "✅ Internet disponible."

  # ======================== VALIDAR TOKEN CON LA API ========================
  log "🔐 Validando Personal Access Token..."
  AUTH_RESPONSE=$(curl -s -H "Authorization: Bearer $GITHUB_TOKEN" "$AUTH_TEST_URL")
  if echo "$AUTH_RESPONSE" | grep -q "message"; then
    error_exit "Token inválido o revocado. Respuesta: $AUTH_RESPONSE"
  fi
  LOGGED_USER=$(echo "$AUTH_RESPONSE" | grep -o '"login":"[^"]*"' | cut -d'"' -f4)
  log "✅ Token válido. Autenticado como: $LOGGED_USER"

  # ======================== VERIFICAR QUE EL REPOSITORIO EXISTE ========================
  log "📦 Verificando existencia del repositorio: $REPO_OWNER/$REPO_NAME"
  REPO_RESPONSE=$(curl -s -H "Authorization: Bearer $GITHUB_TOKEN" "$REPO_URL")
  if echo "$REPO_RESPONSE" | grep -q "Not Found"; then
    cat << EOF
🚨 El repositorio no existe.
💡 Crea uno nuevo en:
   https://github.com/new
   Nombre: $REPO_NAME
   Owner: $REPO_OWNER
EOF
    exit 1
  fi
  log "✅ Repositorio encontrado."

  # ======================== VERIFICAR PERMISOS DE ESCRITURA ========================
  REPO_PERMISSION=$(echo "$REPO_RESPONSE" | grep -o '"permissions":{[^}]*}' | grep -o '"push":true')
  if [ -z "$REPO_PERMISSION" ]; then
    error_exit "❌ No tienes permisos de escritura (push) sobre este repositorio. Revisa los permisos del token."
  fi
  log "✅ Tienes permisos de escritura."

  # ======================== CONFIGURAR REPOSITORIO LOCAL ========================
  if [ ! -d ".git" ]; then
    log "📁 Inicializando repositorio local..."
    safe_run git init
  else
    log "✅ Repositorio local detectado."
  fi

  # Configurar usuario
  safe_run git config user.name "$LOGGED_USER"
  safe_run git config user.email "${LOGGED_USER}@users.noreply.github.com"

  # ======================== CONFIGURAR REMOTO CON TOKEN ========================
  REMOTE_EXISTS=$(git remote | grep origin || true)
  if [ -z "$REMOTE_EXISTS" ]; then
    safe_run git remote add origin "$REMOTE_URL"
    log "🔗 Remoto 'origin' añadido."
  else
    CURRENT_URL=$(git remote get-url origin)
    if [[ "$CURRENT_URL" != *"$REPO_OWNER/$REPO_NAME.git"* ]]; then
      safe_run git remote set-url origin "$REMOTE_URL"
      log "🔄 URL del remoto actualizada."
    fi
  fi

  # ======================== MANEJO DE RAMA: master → main ========================
  if [ -n "$(git status --porcelain)" ]; then
    log "📥 Hay cambios sin guardar. Agregando..."
    git add .
    if ! git diff-index --quiet HEAD --; then
      log "📝 Haciendo commit..."
      git commit -m "📦 Actualización automática $(date '+%Y-%m-%d %H:%M')"
    fi
  fi

  CURRENT_BRANCH=$(git branch --show-current 2>/dev/null || echo "")
  if [ "$CURRENT_BRANCH" = "master" ]; then
    log "🔄 Renombrando rama 'master' → 'main'"
    safe_run git branch -m master main
  elif [ -z "$CURRENT_BRANCH" ]; then
    log "🔧 Creando y cambiando a rama '$TARGET_BRANCH'"
    safe_run git checkout -b "$TARGET_BRANCH"
  elif [ "$CURRENT_BRANCH" != "$TARGET_BRANCH" ]; then
    log "🔧 Cambiando a rama '$TARGET_BRANCH'"
    safe_run git checkout "$TARGET_BRANCH"
  else
    log "✅ En rama correcta: $TARGET_BRANCH"
  fi

  # ======================== HACER PUSH ========================
  log "📤 Enviando cambios a GitHub..."
  if git push -u origin "$TARGET_BRANCH"; then
    log "🎉 ¡Éxito! Cambios subidos a https://github.com/$REPO_OWNER/$REPO_NAME"
  else
    error_exit "Fallo al hacer push. Verifica el token y la conexión."
  fi

  # ======================== FINAL ========================
  echo
} | tee -a "$LOG_FILE"
