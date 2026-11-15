# Broker MQTT – EMQX Cloud

## 1. Présentation

Le broker MQTT assure la communication entre l'ESP32 et le tableau de bord Node-RED.

Après les difficultés rencontrées lors des premiers essais avec les solutions initialement envisagées, le binôme a retenu EMQX Cloud afin d'établir une communication MQTT sécurisée entre les différents composants du système.

Le broker centralise les messages publiés par les clients et les transmet aux clients abonnés aux topics correspondants.

## 2. Configuration du broker

L'instance EMQX Cloud est utilisée comme broker MQTT hébergé dans le cloud.

Paramètres de connexion :

| Paramètre | Configuration |
|---|---|
| Broker | EMQX Cloud |
| Protocole | MQTT over TLS |
| Port | 8883 |
| Authentification | iotuser / imanhafsa123@ |
| Sécurité | TLS avec validation du certificat serveur |

L'adresse de l'hôte est récupérée depuis la console EMQX Cloud.

Les identifiants ne doivent pas être enregistrés dans les fichiers publics du dépôt.

## 3. Sécurisation de la communication

La communication entre les clients et le broker est protégée par TLS.

Les mesures de sécurité utilisées sont :

- Validation du certificat du serveur.
- Utilisation d'identifiants pour chaque client.
- Attribution d'identifiants clients distincts à l'ESP32 et à Node-RED.
- Utilisation de permissions adaptées aux opérations de publication et d'abonnement.
- Utilisation de la racine de certification DigiCert Global Root G2 dans le sketch ESP32.

Si la chaîne de certification du serveur évolue, le certificat CA utilisé doit être vérifié et adapté à partir des informations fournies par EMQX.

## 4. Organisation des topics MQTT

Les échanges entre les composants sont organisés autour des topics suivants :

| Topic | Charge utile | Sens |
|---|---|---|
| `factory/motor/temperature` | Température en °C | ESP32 → Node-RED |
| `factory/motor/vibration` | Accélération en m/s² | ESP32 → Node-RED |
| `factory/motor/status` | ONLINE / OFFLINE | ESP32 → Node-RED |
| `factory/motor/command` | ON / OFF | Node-RED → ESP32 |

Cette organisation permet de séparer les mesures, les informations d'état et les commandes.

## 5. Permissions de communication

Les droits d'accès doivent respecter le rôle de chaque client.

**ESP32 :**
- Publication sur `factory/motor/temperature`.
- Publication sur `factory/motor/vibration`.
- Publication sur `factory/motor/status`.
- Abonnement à `factory/motor/command`.

**Node-RED :**
- Abonnement à `factory/motor/temperature`.
- Abonnement à `factory/motor/vibration`.
- Abonnement à `factory/motor/status`.
- Publication sur `factory/motor/command`.

Les permissions doivent être configurées dans les règles d'autorisation du broker EMQX Cloud.

## 6. Qualité de service et messages retenus

Les niveaux QoS utilisés sont :

- QoS 0 pour la télémétrie.
- QoS 1 pour le Last Will.

Le message Last Will est configuré avec l'option retained afin de conserver le dernier état OFFLINE publié par le broker en cas de déconnexion inattendue de l'ESP32.

Lors d'une reconnexion, l'ESP32 publie un nouveau message ONLINE afin de mettre à jour l'état retenu.

L'abonnement de l'ESP32 aux commandes utilise QoS 0 dans le sketch.

Les commandes ne sont pas retenues (retain désactivé) afin d'éviter l'exécution d'une ancienne commande lors d'une nouvelle connexion.

## 7. Gestion des déconnexions

Le mécanisme Last Will permet au broker de publier automatiquement un message OFFLINE lorsque la connexion de l'ESP32 est interrompue de manière inattendue.

Lors du rétablissement de la connexion, l'ESP32 publie ONLINE.

Le statut est ainsi transmis à Node-RED, qui peut afficher l'état de disponibilité du système.

## 8. Vérification de la communication

Les vérifications à effectuer comprennent :

- Connexion de l'ESP32 au broker avec authentification.
- Connexion de Node-RED au même broker.
- Réception des mesures publiées par l'ESP32.
- Réception des commandes par l'ESP32.
- Vérification du statut ONLINE.
- Simulation d'une interruption réseau pour vérifier le Last Will.
- Vérification du rétablissement de la communication après reconnexion.
- Vérification du rejet d'une connexion sans identifiants valides.

## 9. Fichiers du dossier

La configuration du broker étant hébergée sur EMQX Cloud, aucun serveur MQTT local n'est nécessaire pour l'exécution du prototype.

Ce dossier contient principalement :

- `README.md` : documentation de configuration et de fonctionnement du broker MQTT.

Les paramètres sensibles sont conservés hors du dépôt.