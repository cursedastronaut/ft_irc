# Toolbox — PROTO-07 (#25) : ClientRegistry

## Fichiers

- `include/protocol/ClientRegistry.hpp` (déclarations)
- `src/protocol/ClientRegistry.cpp` (implémentation, à écrire)

## Contexte

Aucune classe ne implémente encore `protocol`/`transport`
(`include/contracts/Protocol.hpp`) dans le repo — seulement des stubs de
test. `ClientRegistry` est donc un composant **autonome**, pas branché sur
du code existant : il sera utilisé plus tard par la classe qui implémentera
`protocol::connected()`.

Ne pas confondre avec `ClientIdentity` (même fichier `Protocol.hpp`,
`nickname`/`username`/`host`) : c'est côté `transport` (NET), lié aux
sockets. `ClientRecord` n'a rien à voir, c'est un bookkeeping interne à
`protocol`, décidé minimal pour l'instant.

## Types disponibles

```cpp
#include "contracts/ClientId.hpp"
// typedef unsigned long irc::ClientId;
```

## Outils std à connaître

- `std::map<K, V>` (`<map>`)
  - `_records[id] = record;` — insère ou écrase
  - `_records.insert(std::make_pair(id, record));` — insère seulement si
    absent (ne modifie rien si la clé existe déjà)
  - `std::map<K,V>::iterator it = _records.find(id);`
    `it == _records.end()` → absent ; sinon `it->second` est la valeur
  - `_records.erase(id);` — retire la clé si présente, **ne proteste pas**
    si elle est absente (renvoie juste `0`) → base idéale pour un `remove`
    "safe à rappeler"

## Signatures à écrire

```cpp
namespace irc
{
	struct ClientRecord
	{
		ClientId	id;
	};

	class ClientRegistry
	{
		public:
			void	create(ClientId id);
			bool	find(ClientId id, ClientRecord &out) const;
			void	remove(ClientId id);

		private:
			std::map<ClientId, ClientRecord>	_records;
	};
}
```

- `create` : construit un `ClientRecord` et l'ajoute à `_records` pour cet
  `id`. (Le contrat NET garantit qu'un `ClientId` n'est jamais réutilisé,
  donc pas besoin de gérer un doublon comme un cas normal — mais réfléchis
  à ce que `insert` fait déjà si jamais ça arrivait.)
- `find` : cherche dans `_records`, si trouvé copie dans `out` et renvoie
  `true`, sinon renvoie `false` sans toucher `out`.
- `remove` : retire l'entrée si elle existe, ne fait rien sinon.

## Contrainte à respecter (CORE-04)

Aucun champ fd/socket dans `ClientRecord`, aucun appel à `close()` nulle
part dans `ClientRegistry` — le destructeur du registre ne doit fermer
aucun descripteur. Ça se vérifie par relecture, pas par un test.

## Critères d'acceptation — PROTO-07 (#25)

- [ ] Each ClientId has exactly one record
- [ ] Repeated removal is safe and protocol destruction never closes a
      descriptor
