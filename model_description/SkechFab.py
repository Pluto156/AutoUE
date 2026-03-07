import os
import time
import json
import zipfile
import requests

SKETCHFAB_API = "https://api.sketchfab.com/v3/models"
SKETCHFAB_TOKEN = os.getenv("SKETCHFAB_TOKEN")  # Token must be set

if not SKETCHFAB_TOKEN:
    raise RuntimeError("Please set the SKETCHFAB_TOKEN environment variable")

HEADERS = {
    "Authorization": f"Token {SKETCHFAB_TOKEN}"
}

# ============================================================
# Default download directory (used when no path is provided)
# ============================================================
# Base root directory from environment variable
ROOT_DIR = os.getenv("ROOT_DIR")
if not ROOT_DIR:
    raise RuntimeError("Please set ROOT_DIR environment variable.")

# Default download directory for glTF models
DEFAULT_DOWNLOAD_DIR = os.path.join(ROOT_DIR, "model_description", "gltf")


# ------------------------------------------------------------
# Request download information (mimics Blender plugin behavior)
# ------------------------------------------------------------
def get_download_url(uid):
    url = f"{SKETCHFAB_API}/{uid}/download"
    print(f"[DEBUG] Requesting download info: {url}")

    r = requests.get(url, headers=HEADERS)
    print(f"[DEBUG] HTTP status code: {r.status_code}")

    if r.status_code != 200:
        print("[ERROR] Model is not downloadable or permission denied")
        print(r.text)
        return None

    data = r.json()
    print("[DEBUG] Download JSON fields:", list(data.keys()))

    if "gltf" not in data:
        print("[ERROR] No gltf field in JSON, model is not downloadable")
        return None

    return data["gltf"]["url"]


# ------------------------------------------------------------
# Download ZIP and save to disk (path injectable)
# ------------------------------------------------------------
def download_zip(url, uid, download_dir):
    model_dir = os.path.join(download_dir, uid)
    os.makedirs(model_dir, exist_ok=True)

    zip_path = os.path.join(model_dir, f"{uid}.zip")
    print(f"[DEBUG] ZIP will be saved to: {zip_path}")

    if os.path.exists(zip_path):
        print("[INFO] File already exists, skipping download")
        return zip_path

    with requests.get(url, stream=True) as r:
        r.raise_for_status()
        total = r.headers.get("content-length")

        with open(zip_path, "wb") as f:
            if total is None:
                f.write(r.content)
            else:
                dl = 0
                total = int(total)
                for chunk in r.iter_content(chunk_size=4096):
                    if not chunk:
                        continue
                    f.write(chunk)
                    dl += len(chunk)
                    done = int(50 * dl / total)
                    print(
                        "\r[Downloading] [{}{}] {}%".format(
                            "=" * done,
                            " " * (50 - done),
                            int(dl * 100 / total)
                        ),
                        end=""
                    )

    print("\n[INFO] Download completed")
    return zip_path


# ------------------------------------------------------------
# Extract ZIP and attempt to locate FBX files
# ------------------------------------------------------------
def extract_zip(zip_path, uid):
    extract_dir = os.path.join(os.path.dirname(zip_path), "extracted")
    os.makedirs(extract_dir, exist_ok=True)

    print(f"[DEBUG] Extract path: {extract_dir}")

    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(extract_dir)

    print("[INFO] ZIP extraction completed")

    fbx_files = []
    for root, dirs, files in os.walk(extract_dir):
        for f in files:
            if f.lower().endswith(".fbx"):
                fbx_files.append(os.path.join(root, f))

    if fbx_files:
        print("[✔] FBX files found:")
        for f in fbx_files:
            print("   ", f)
    else:
        print("[×] No FBX files found in ZIP (may contain only glTF)")

    return extract_dir, fbx_files


# ------------------------------------------------------------
# Main flow for a single model (path optional)
# ------------------------------------------------------------
def download_model(uid, download_dir=None):
    """
    Download a Sketchfab model.

    :param uid: Sketchfab model UID
    :param download_dir: Target directory.
                         If None, DEFAULT_DOWNLOAD_DIR is used.
    """
    if download_dir is None:
        download_dir = DEFAULT_DOWNLOAD_DIR

    print("\n==============================")
    print(f"Processing model: {uid}")
    print(f"[INFO] Download directory: {download_dir}")

    os.makedirs(download_dir, exist_ok=True)

    download_url = get_download_url(uid)
    if not download_url:
        print("[ERROR] No valid download URL")
        return

    zip_path = download_zip(download_url, uid, download_dir)
    extract_dir, fbx_files = extract_zip(zip_path, uid)

    print(f"[DONE] Model {uid} processed successfully")


# ------------------------------------------------------------
# Batch download (still uses default path)
# ------------------------------------------------------------
def download_models_from_uid_list(uid_list):
    # =============================
    # Clear all contents under DEFAULT_DOWNLOAD_DIR
    # =============================
    if os.path.exists(DEFAULT_DOWNLOAD_DIR):
        for root, dirs, files in os.walk(DEFAULT_DOWNLOAD_DIR, topdown=False):
            for f in files:
                try:
                    os.remove(os.path.join(root, f))
                except Exception as e:
                    print(f"[WARN] Failed to delete file: {f} | {e}")
            for d in dirs:
                try:
                    os.rmdir(os.path.join(root, d))
                except Exception as e:
                    print(f"[WARN] Failed to delete directory: {d} | {e}")
        print(f"[INFO] Cleared directory: {DEFAULT_DOWNLOAD_DIR}")

    os.makedirs(DEFAULT_DOWNLOAD_DIR, exist_ok=True)

    # =============================
    # Start batch download
    # =============================
    for uid in uid_list:
        download_model(uid)  # default path
        time.sleep(0.2)

    print("\nAll models have been processed")


# ------------------------------------------------------------
# Entry point (example UIDs)
# ------------------------------------------------------------
if __name__ == "__main__":
    uid_list = [
        "00001dbc99db48efb16f81945dcc9999",
        "00001e5e32664335abb5e207b81a9f0a",
        "0000304b89ca43ea9d63a7c32f0398bc",
        "000054a8d92d4c80828161fbb235d141",
        "000074a334c541878360457c672b6c2e",
        "0000954cf268495ca39760d3d8e11862",
        "0000ae10c2a24d5d90807593d6997f6f",
        "0000c32fde7f45efb8d14e8ba737d50c",
        "0000d20857dd4651a0bd73a62d6fe155",
        "0000d2a30f2d4626bd1d5e765634502a"
    ]

    download_models_from_uid_list(uid_list)
