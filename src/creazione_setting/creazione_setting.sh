#!/bin/bash
set -euo pipefail

# =========================
# CONFIGURAZIONE
# =========================

# URL e nomi dei 3 wheel (aggiungi ?download=1 per link Box)
WHEEL_URLS=(
  "https://nvidia.box.com/shared/static/mp164asf3sceb570wvjsrezk1p4ftj8t.whl?download=1"
  "https://nvidia.box.com/shared/static/xpr06qe6ql3l6rj22cu3c45tz1wzi36p.whl?download=1"
  "https://nvidia.box.com/shared/static/9agsjfee0my4sxckdpuk9x9gt8agvjje.whl?download=1"
)

WHEEL_NAMES=(
  "torch-2.3.0-cp310-cp310-linux_aarch64.whl"
  "torchvision-0.18.0-cp310-cp310-linux_aarch64.whl"
  "torchaudio-2.3.0-cp310-cp310-linux_aarch64.whl"
)

# === Directory del progetto (dove si trova lo script)
PROJ_DIR="$(pwd)"
REQS_DIR="$PROJ_DIR/reqs"
WHEELS_DIR="$PROJ_DIR/downloads"

# === Directory ESTERNE da creare
PARENT_DIR="$(dirname "$PROJ_DIR")"
BASE1="$PARENT_DIR/Artes_Audio1"
BASE2="$PARENT_DIR/Artes_Video1"
ENV1_DIR="$BASE1/artes_audio"
ENV2_DIR="$BASE2/artes_video"

# === Percorsi ai file requirements
REQ1="$REQS_DIR/requirements_audio.txt"
REQ2="$REQS_DIR/requirements_video.txt"

# =========================
# CONTROLLI
# =========================
if [ ${#WHEEL_URLS[@]} -ne ${#WHEEL_NAMES[@]} ]; then
  echo "Errore: WHEEL_URLS e WHEEL_NAMES devono avere la stessa lunghezza." >&2
  exit 1
fi

# =========================
# PREPARAZIONE CARTELLE
# =========================
mkdir -p "$REQS_DIR" "$WHEELS_DIR" "$BASE1" "$BASE2"
echo ">>> Struttura creata:"
echo " - reqs/: $REQS_DIR"
echo " - downloads/: $WHEELS_DIR"
echo " - Artes_Audio1/: $BASE1"
echo " - Artes_Video1/: $BASE2"

# =========================
# DOWNLOAD WHEEL
# =========================
echo ">>> Scarico ${#WHEEL_URLS[@]} wheel in $WHEELS_DIR/"
for i in "${!WHEEL_URLS[@]}"; do
  url="${WHEEL_URLS[$i]}"
  name="${WHEEL_NAMES[$i]}"
  echo " - $name"
  curl -L -o "$WHEELS_DIR/$name" "$url"
  if ! file "$WHEELS_DIR/$name" | grep -qi "Zip archive data"; then
    echo "ATTENZIONE: '$name' non sembra un wheel valido (zip)" >&2
  fi
done

# =========================
# FUNZIONE: merge requirements
# =========================
merge_requirements() {
  local req_file="$1"
  local tmp="${req_file}.tmp"
  : > "$tmp"
  for name in "${WHEEL_NAMES[@]}"; do
    echo "./downloads/$name" >> "$tmp"
  done
  if [ -f "$req_file" ]; then
    cat "$req_file" >> "$tmp"
  fi
  awk '!seen[$0]++' "$tmp" > "$req_file"
  rm -f "$tmp"
}

# =========================
# COSTRUZIONE REQUIREMENTS
# =========================
echo ">>> Aggiorno requirements"
merge_requirements "$REQ1"
merge_requirements "$REQ2"

echo "---- $REQ1 ----"
cat "$REQ1" || true
echo "----------------"
echo "---- $REQ2 ----"
cat "$REQ2" || true
echo "----------------"

# =========================
# CREA E INSTALLA ENV1
# =========================
echo ">>> Creo ambiente virtuale: $ENV1_DIR"
python3 -m venv "$ENV1_DIR"
source "$ENV1_DIR/bin/activate"
python -m pip install --upgrade pip wheel setuptools
pip install --no-cache-dir -r "$REQ1"
deactivate
echo ">>> Disattivato artes_audio"

# =========================
# CREA E INSTALLA ENV2
# =========================
echo ">>> Creo ambiente virtuale: $ENV2_DIR"
python3 -m venv "$ENV2_DIR"
source "$ENV2_DIR/bin/activate"
python -m pip install --upgrade pip wheel setuptools
pip install --no-cache-dir -r "$REQ2"
deactivate
echo ">>> Disattivato artes_video"

echo ">>> COMPLETATO!"
echo "    - artes_audio: $ENV1_DIR"
echo "    - artes_video: $ENV2_DIR"
echo "    - wheel: $WHEELS_DIR"
echo "    - requirements: $REQS_DIR"

