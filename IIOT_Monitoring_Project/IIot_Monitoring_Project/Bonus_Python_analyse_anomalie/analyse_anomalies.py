
import csv
import math
import statistics
from collections import deque
from pathlib import Path

# ===== CONFIGURATION =====
TEMP_LIMIT = 40.0
VIB_LIMIT = 4.0

WINDOW_SIZE = 5
K = 3.0

# Cherche le CSV dans le meme dossier que ce script
DOSSIER = Path(__file__).resolve().parent
CSV_FILE = DOSSIER / "motor-history.csv"
RESULTS_DIR = DOSSIER / "resultats"
RESULTS_DIR.mkdir(exist_ok=True)


# ===== LECTURE DES DONNEES NODE-RED =====
def charger_mesures():

    with open(CSV_FILE, newline="", encoding="utf-8-sig") as f:
        lignes = list(csv.DictReader(f))

    # Node-RED enregistre plusieurs evenements par cycle :
    # TEMPERATURE, VIBRATION, ALERT.
    # On conserve la derniere alerte de chaque cycle.

    groupes = []
    groupe = []

    for ligne in lignes:

        if ligne["Type"] == "TEMPERATURE" and groupe:
            groupes.append(groupe)
            groupe = []

        groupe.append(ligne)

    if groupe:
        groupes.append(groupe)

    mesures = []

    for groupe in groupes:

        alertes = [
            ligne for ligne in groupe
            if ligne["Type"] == "ALERT"
        ]

        if not alertes:
            continue

        derniere = alertes[-1]

        try:
            temperature = float(derniere["Temperature"])
            vibration = float(derniere["Vibration"])

            if not (
                math.isfinite(temperature)
                and math.isfinite(vibration)
            ):
                continue

            mesures.append({
                "timestamp": derniere["Timestamp"],
                "temperature": temperature,
                "vibration": vibration,
                "statut": derniere["Value"]
            })

        except (ValueError, TypeError):
            continue

    return lignes, mesures


# ===== DETECTION DES ANOMALIES =====
def detecter_anomalies(valeurs, seuil, ecart_min):

    historique = deque(maxlen=WINDOW_SIZE)

    anomalies = []
    raisons = []

    for valeur in valeurs:

        # 1. Detection par seuil fixe
        seuil_depasse = valeur >= seuil

        # 2. Detection par moyenne mobile et ecart-type
        variation_anormale = False

        if len(historique) == WINDOW_SIZE:

            moyenne = statistics.mean(historique)

            ecart_type = max(
                statistics.pstdev(historique),
                ecart_min
            )

            seuil_adaptatif = moyenne + K * ecart_type

            variation_anormale = valeur > seuil_adaptatif

        anomalie = seuil_depasse or variation_anormale

        anomalies.append(anomalie)

        if seuil_depasse:
            raisons.append("Seuil depasse")
        elif variation_anormale:
            raisons.append("Variation statistique anormale")
        else:
            raisons.append("Normal")

        # Eviter que les anomalies modifient
        # la reference des mesures normales
        if not anomalie:
            historique.append(valeur)

    return anomalies, raisons


# ===== GRAPHIQUES =====
def creer_graphique(valeurs, anomalies, seuil, titre, unite, fichier):

    import matplotlib.pyplot as plt

    x = list(range(1, len(valeurs) + 1))

    plt.figure(figsize=(10, 4))

    plt.plot(
        x,
        valeurs,
        marker="o",
        label="Mesures du moteur"
    )

    plt.axhline(
        seuil,
        linestyle="--",
        label=f"Seuil = {seuil} {unite}"
    )

    indices = [
        i for i, anomalie in enumerate(anomalies)
        if anomalie
    ]

    plt.scatter(
        [x[i] for i in indices],
        [valeurs[i] for i in indices],
        marker="x",
        s=80,
        label="Anomalies detectees"
    )

    plt.title(titre)
    plt.xlabel("Cycle de mesure")
    plt.ylabel(unite)
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    plt.savefig(RESULTS_DIR / fichier, dpi=150)
    plt.close()


# ===== PROGRAMME PRINCIPAL =====
def main():

    if not CSV_FILE.exists():
        print("ERREUR : motor-history.csv introuvable !")
        return

    lignes, mesures = charger_mesures()

    if not mesures:
        print("Aucune mesure complete trouvee.")
        return

    temperatures = [m["temperature"] for m in mesures]
    vibrations = [m["vibration"] for m in mesures]

    # Anomalies temperature
    alertes_temp, raisons_temp = detecter_anomalies(
        temperatures,
        TEMP_LIMIT,
        ecart_min=0.5
    )

    # Anomalies vibration
    alertes_vib, raisons_vib = detecter_anomalies(
        vibrations,
        VIB_LIMIT,
        ecart_min=0.15
    )

    # ===== ENREGISTRER LES RESULTATS =====
    fichier_resultat = RESULTS_DIR / "anomalies_detectees.csv"

    with open(
        fichier_resultat, "w",
        newline="", encoding="utf-8"
    ) as f:

        writer = csv.writer(f)

        writer.writerow([
            "timestamp",
            "temperature_c",
            "vibration_ms2",
            "anomalie_temperature",
            "anomalie_vibration",
            "raison_temperature",
            "raison_vibration",
            "statut_nodered"
        ])

        for i, m in enumerate(mesures):

            writer.writerow([
                m["timestamp"],
                m["temperature"],
                m["vibration"],
                int(alertes_temp[i]),
                int(alertes_vib[i]),
                raisons_temp[i],
                raisons_vib[i],
                m["statut"]
            ])

    # ===== AFFICHER LES RESULTATS =====
    total_anomalies = sum(
        t or v for t, v in zip(alertes_temp, alertes_vib)
    )

    print("\n===== ANALYSE IIoT =====")
    print("Evenements Node-RED :", len(lignes))
    print("Cycles de mesure :", len(mesures))
    print("Anomalies temperature :", sum(alertes_temp))
    print("Anomalies vibration :", sum(alertes_vib))
    print("Echantillons anormaux :", total_anomalies)

    print("\nResultat CSV :", fichier_resultat)

    # ===== GENERER LES GRAPHIQUES =====
    try:
        creer_graphique(
            temperatures,
            alertes_temp,
            TEMP_LIMIT,
            "Temperature du moteur",
            "°C",
            "graphique_temperature.png"
        )

        creer_graphique(
            vibrations,
            alertes_vib,
            VIB_LIMIT,
            "Indicateur de vibration",
            "m/s²",
            "graphique_vibration.png"
        )

        print("Graphiques generes dans le dossier resultats.")

    except ImportError:
        print("Installe matplotlib : pip install matplotlib")


if __name__ == "__main__":
    main()
