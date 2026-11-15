# Tableau de bord Node-RED

## 1. Présentation

Cette partie du projet est consacrée à la supervision du moteur industriel à travers un tableau de bord développé avec Node-RED.

Le dashboard reçoit les données publiées par l'ESP32 via le broker MQTT EMQX Cloud. Il permet de visualiser les mesures en temps réel, de suivre leur évolution et de superviser l'état du moteur.

## 2. Installation et configuration

Node-RED doit être installé sur la machine utilisée pour exécuter le dashboard.

Lancement de Node-RED :

```bash
node-red
```

Accéder ensuite à l'éditeur depuis un navigateur :

```text
http://localhost:1880
```

## 3. Importation du flow

Le fichier `flows.json` contient la configuration du dashboard Node-RED.

Pour l'importer :

1. Ouvrir l'éditeur Node-RED.
2. Accéder au menu principal.
3. Sélectionner Import.
4. Charger le fichier `flows.json`.
5. Installer les éventuels nœuds supplémentaires signalés par Node-RED.
6. Vérifier la configuration des nœuds MQTT.
7. Déployer le flow.

## 4. Configuration de la communication MQTT

La connexion au broker EMQX Cloud utilise les paramètres suivants :

- Protocole : MQTT over TLS.
- Port : 8883.
- Adresse : hôte de l'instance EMQX Cloud.
- Authentification : nom d'utilisateur et mot de passe.
- Validation du certificat TLS : activée.

Les identifiants doivent être renseignés directement dans la configuration du client MQTT Node-RED. Ils ne doivent pas être publiés dans ce dépôt.

Le dashboard utilise un identifiant client distinct de celui de l'ESP32.

## 5. Topics MQTT utilisés

Node-RED s'abonne aux topics suivants :

| Topic | Fonction |
|---|---|
| `factory/motor/temperature` | Réception de la température |
| `factory/motor/vibration` | Réception des vibrations |
| `factory/motor/status` | Suivi de l'état de connexion |
| `factory/motor/command` | Publication des commandes |

Les mesures et les informations d'état sont reçues depuis l'ESP32. Les commandes sont publiées par Node-RED à destination de l'ESP32.

## 6. Fonctionnalités du dashboard

Le tableau de bord permet de superviser le fonctionnement du moteur à travers :

- L'affichage des mesures de température et de vibration.
- Le suivi de l'évolution des mesures.
- La détection et l'affichage des alertes.
- La visualisation de l'état de connexion MQTT.
- L'envoi de commandes au moteur.
- L'enregistrement et la consultation de l'historique des mesures et des alertes, selon les fonctions configurées dans le flow.

## 7. Historisation des données

Les données reçues sont traitées par le flow Node-RED et peuvent être enregistrées dans des fichiers CSV.

Les fichiers d'historique doivent être conservés sur la machine qui héberge Node-RED. Les chemins d'enregistrement doivent être vérifiés et adaptés à l'environnement d'exécution.

Chaque enregistrement doit comporter un horodatage permettant de retrouver le moment de réception de la mesure ou de génération de l'alerte.

## 8. Vérifications

Avant de considérer le dashboard opérationnel, vérifier :

- L'établissement de la connexion au broker EMQX Cloud.
- La réception des mesures de température et de vibration.
- La mise à jour de l'état MQTT.
- La génération des alertes lorsque les seuils configurés sont dépassés.
- L'envoi et la réception des commandes.
- L'enregistrement correct des données dans les fichiers CSV.

## 9. Fichiers du dossier

- `flows.json` : export du flow Node-RED utilisé pour la supervision du moteur.
- `README.md` : documentation d'installation, de configuration et d'utilisation du dashboard.
