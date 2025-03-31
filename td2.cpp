// Solutionnaire du TD4 INF1015 hiver 2025
// Par Francois-R.Boyer@PolyMtl.ca

#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include <sstream>
#include "cppitertools/range.hpp"
#include "cppitertools/enumerate.hpp"
#include <span>
#include <forward_list>

#if __has_include("gtest/gtest.h")
#include "gtest/gtest.h"
#endif

#if __has_include("bibliotheque_cours.hpp")
#include "bibliotheque_cours.hpp"
#define BIBLIOTHEQUE_COURS_INCLUS
using bibliotheque_cours::cdbg;
#else
auto& cdbg = clog;
#endif

#if __has_include("verification_allocation.hpp")
#include "verification_allocation.hpp"
#include "debogage_memoire.hpp"  //NOTE: Incompatible avec le "placement new", ne pas utiliser cette entête si vous utilisez ce type de "new" dans les lignes qui suivent cette inclusion.
#endif

void initialiserBibliothequeCours([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
#ifdef BIBLIOTHEQUE_COURS_INCLUS
	bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

	// cdbg.setTee(&clog);  // Décommenter cette ligne pour que cdbg affiche sur la console en plus de la "Sortie" du débogueur.

	bibliotheque_cours::executerGoogleTest(argc, argv); // Attention de ne rien afficher avant cette ligne, sinon l'Explorateur de tests va tenter de lire votre affichage comme un résultat de test.
#endif
}

using namespace std;
using namespace iter;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{

UInt8 lireUint8(istream& fichier)
{
	UInt8 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
UInt16 lireUint16(istream& fichier)
{
	UInt16 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUint16(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion//}

// Fonctions pour ajouter un Film à une ListeFilms.
//[
void ListeFilms::changeDimension(int nouvelleCapacite)
{
	Film** nouvelleListe = new Film * [nouvelleCapacite];

	if (elements != nullptr) {  // Noter que ce test n'est pas nécessaire puique nElements sera zéro si elements est nul, donc la boucle ne tentera pas de faire de copie, et on a le droit de faire delete sur un pointeur nul (ça ne fait rien).
		nElements = min(nouvelleCapacite, nElements);
		for (int i : range(nElements))
			nouvelleListe[i] = elements[i];
		delete[] elements;
	}

	elements = nouvelleListe;
	capacite = nouvelleCapacite;
}

void ListeFilms::ajouterFilm(Film* film)
{
	if (nElements == capacite)
		changeDimension(max(1, capacite * 2));
	elements[nElements++] = film;
}
//]

// Fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; la fonction prenant en paramètre un pointeur vers le film à enlever.  L'ordre des films dans la liste n'a pas à être conservé.
//[
// On a juste fait une version const qui retourne un span non const.  C'est valide puisque c'est la struct qui est const et non ce qu'elle pointe.  Ça ne va peut-être pas bien dans l'idée qu'on ne devrait pas pouvoir modifier une liste const, mais il y aurais alors plusieurs fonctions à écrire en version const et non-const pour que ça fonctionne bien, et ce n'est pas le but du TD (il n'a pas encore vraiment de manière propre en C++ de définir les deux d'un coup).
span<Film*> ListeFilms::enSpan() const { return span(elements, nElements); }

//void ListeFilms::enleverFilm(const Film* film)  // Pas utile dans ce TD
//{
//	for (Film*& element : enSpan()) {  // Doit être une référence au pointeur pour pouvoir le modifier.
//		if (element == film) {
//			if (nElements > 1)
//				element = elements[nElements - 1];
//			nElements--;
//			return;
//		}
//	}
//}
//]

// Fonction pour trouver un Acteur par son nom dans une ListeFilms, qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé.  Devrait utiliser span.
//[

//NOTE: Doit retourner un Acteur modifiable, sinon on ne peut pas l'utiliser pour modifier l'acteur tel que demandé dans le main, et on ne veut pas faire écrire deux versions.
shared_ptr<Acteur> ListeFilms::trouverActeur(const string& nomActeur) const
{
	for (const Film* film : enSpan()) {
		for (const shared_ptr<Acteur>& acteur : film->acteurs.enSpan()) {
			if (acteur->nom == nomActeur)
				return acteur;
		}
	}
	return nullptr;
}
//]

// Les fonctions pour lire le fichier et créer/allouer une ListeFilms.

shared_ptr<Acteur> lireActeur(istream& fichier, const ListeFilms& listeFilms)
{
	Acteur acteur = {};
	acteur.nom = lireString(fichier);
	acteur.anneeNaissance = lireUint16(fichier);
	acteur.sexe = lireUint8(fichier);

	shared_ptr<Acteur> acteurExistant = listeFilms.trouverActeur(acteur.nom);
	if (acteurExistant != nullptr)
		return acteurExistant;
	else {
		cout << "Création Acteur " << acteur.nom << endl;
		return make_shared<Acteur>(move(acteur));  // Le move n'est pas nécessaire mais permet de transférer le texte du nom sans le copier.
	}
}

Film* lireFilm(istream& fichier, ListeFilms& listeFilms)
{
	Film* film = new Film;
	film->titre = lireString(fichier);
	film->realisateur = lireString(fichier);
	film->anneeSortie = lireUint16(fichier);
	film->recette = lireUint16(fichier);
	auto nActeurs = lireUint8(fichier);
	film->acteurs = ListeActeurs(nActeurs);  // On n'a pas fait de méthode pour changer la taille d'allocation, seulement un constructeur qui prend la capacité.  Pour que cette affectation fonctionne, il faut s'assurer qu'on a un operator= de move pour ListeActeurs.
	cout << "Création Film " << film->titre << endl;

	for ([[maybe_unused]] auto i : range(nActeurs)) {  // On peut aussi mettre nElements avant et faire un span, comme on le faisait au TD précédent.
		film->acteurs.ajouter(lireActeur(fichier, listeFilms));
	}

	return film;
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);

	int nElements = lireUint16(fichier);

	ListeFilms listeFilms;
	for ([[maybe_unused]] int i : range(nElements)) { //NOTE: On ne peut pas faire un span simple avec ListeFilms::enSpan car la liste est vide et on ajoute des éléments à mesure.
		listeFilms.ajouterFilm(lireFilm(fichier, listeFilms));
	}

	return listeFilms;
}

// Fonction pour détruire une ListeFilms et tous les films qu'elle contient.
//[
// On détruit sans détruire les films. On n'a pas demandé de refaire la lecture des films directement avec des pointeurs intelligents. On n'a pas demandé non plus de remplacer la méthode "detruire" par un destructeur.
void ListeFilms::detruire()
{
	//if (possedeLesFilms)  // Pas utile dans ce TD.
	//	for (Film* film : enSpan())
	//		delete film;
	delete[] elements;
}
//]

// Pour que l'affichage de Film fonctionne avec <<, il faut aussi modifier l'affichage de l'acteur pour avoir un ostream; l'énoncé ne demande pas que ce soit un opérateur, mais tant qu'à y être...
ostream& operator<< (ostream& os, const Acteur& acteur)
{
	return os << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

// Fonctions pour afficher les Item, Film, Livre ...
//[
ostream& operator<< (ostream& os, const Affichable& affichable)
{
	affichable.afficherSur(os);
	return os;
}

void Item::afficherSur(ostream& os) const
{
	os << "Titre: " << titre << "  Année:" << anneeSortie << endl;
}

void Film::afficherSpecifiqueSur(ostream& os) const
{
	os << "  Réalisateur: " << realisateur << endl;
	os << "  Recette: " << recette << "M$" << endl;
	os << "Acteurs:" << endl;
	for (auto&& acteur : acteurs.enSpan())
		os << *acteur;
}

void Film::afficherSur(ostream& os) const
{
	Item::afficherSur(os);
	Film::afficherSpecifiqueSur(os);
}

void Livre::afficherSpecifiqueSur(ostream& os) const
{
	os << "  Auteur: " << auteur << endl;
	os << "  Vendus: " << copiesVendues << "M  Pages: " << nPages << endl;
}

void Livre::afficherSur(ostream& os) const
{
	Item::afficherSur(os);
	Livre::afficherSpecifiqueSur(os);
}

void FilmLivre::afficherSur(ostream& os) const
{
	Item::afficherSur(os);
	os << "Combo:" << endl;
	// L'affichage comme l'exemple sur Discord est accepté, ici on montre comment on pourrait séparer en deux méthodes pour ne pas avoir le même titre d'Item affiché plusieurs fois.
	Film::afficherSpecifiqueSur(os);
	os << "Livre:" << endl;
	Livre::afficherSpecifiqueSur(os);
}

//]

// Pourrait être une méthode static pour construire un Livre à partir des données du fichier (pas encore vu les méthodes static dans le cours), ou un constructeur avec tag.  On a fait un constructeur explicit pour ne pas qu'un istream puisse être converti implicitement en livre, mais le tag n'était pas nécessaire puisqu'on avait une seule version de ce constructeur.  On a aussi décidé de faire une méthode pour lire (qui pourrait être utilisée par un opérateur, mais pas nécessaire ici).  La méthode pourrait être virtuelle si on avait besoin de faire la lecture selon le type dynamique mais ici on sais le type statiquement.
void Item::lireDe(istream& is)
{
	is >> quoted(titre) >> anneeSortie;
}
void Livre::lireDe(istream& is)
{
	Item::lireDe(is);
	is >> quoted(auteur) >> copiesVendues >> nPages;
}
Livre::Livre(istream& is) {
	lireDe(is);
}
template <typename Container>
void afficherListeItems(const Container& listeItems)
{
	static const string ligneDeSeparation = "\033[32m────────────────────────────────────────\033[0m\n";
	cout << ligneDeSeparation;
	for (const auto& itemPtr : listeItems) {
		cout << itemPtr->titre << ligneDeSeparation;
	}
}

#pragma region "Exemples de tests unitaires"//{
#ifdef TEST
// Pas demandés dans ce TD mais sert d'exemple.

TEST(tests_ListeFilms, tests_simples) {
	ListeFilms li;
	EXPECT_EQ(li.size(), 0);
	EXPECT_EQ(li.capacity(), 0);
	Film a, b, c;
	li.ajouterFilm(&a);
	li.ajouterFilm(&b);
	li.ajouterFilm(&c);
	EXPECT_EQ(li.size(), 3);
	EXPECT_GE(li.capacity(), 3);
	EXPECT_EQ(li[0], &a);
	EXPECT_EQ(li[1], &b);
	EXPECT_EQ(li[2], &c);
	li.detruire();
}

template <> struct Film::accessible_pour_tests_par<::testing::Test> {
	template <typename T> // Template pour ne pas avoir une version const et non const.
	static auto& realisateur(T&& film) { return film.realisateur; }
};

TEST(tests_ListeFilms, trouver) {
	using AccesFilmPourTests = Film::accessible_pour_tests_par<::testing::Test>;
	ListeFilms li;
	Film films[3];
	string realisateurs[] = { "a","b","c","e" };
	for (auto&& [i, f] : enumerate(films)) {
		AccesFilmPourTests::realisateur(f) = realisateurs[i];
		li.ajouterFilm(&f);
	}
	for (auto&& [i, r] : enumerate(realisateurs)) {
		Film* film = li.trouver([&](const Film& f) { return AccesFilmPourTests::realisateur(f) == r; });
		// Le << après EXPECT_... permet d'afficher quelque chose en cas d'échec du test. Dans ce cas-ci, on veut savoir pour quel i ça a échoué. Noter que gtest garantit que cette partie après n'est pas évaluée/exécutée dans le cas où la condition du EXPECT_... est validée.
		EXPECT_EQ(film, i < size(films) ? &films[i] : nullptr) << "  pour i=" << i;
	}
	li.detruire();
}

#endif
#pragma endregion//}

int main(int argc, char* argv[])
{
	initialiserBibliothequeCours(argc, argv);

	//int* fuite = new int; //TODO: Enlever cette ligne après avoir vérifié qu'il y a bien un "Detected memory leak" de "4 bytes" affiché dans la "Sortie", qui réfère à cette ligne du programme.

	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	vector<unique_ptr<Item>> items;

	{
		ListeFilms listeFilms = creerListe("films.bin");
		for (auto&& film : listeFilms.enSpan())
			items.push_back(unique_ptr<Item>(film));  // On transert la possession.
		listeFilms.detruire();
	}

	{
		ifstream fichier("livres.txt");
		fichier.exceptions(ios::failbit);  // Pas demandé mais permet de savoir s'il y a une erreur de lecture.
		while (!ws(fichier).eof())
			items.push_back(make_unique<Livre>(fichier));
	}

	items.push_back(make_unique<FilmLivre>(dynamic_cast<Film&>(*items[4]), dynamic_cast<Livre&>(*items[9])));  // On ne demandait pas de faire une recherche; serait direct avec la matière du TD5.

	afficherListeItems(items);
	// Question 1.1
	forward_list<unique_ptr<Item>> items1;
	items1.push_front(move(items[0]));
	auto fl_index = items1.begin();
	advance(fl_index, 0);

	for (int i = 1; i < items.size(); i++) {
		items1.insert_after(fl_index, move(items[i]));
		advance(fl_index, i);
	}
	cout << "Liste 1" << endl;
	afficherListeItems(items1);

	//Question 1.2
	forward_list <unique_ptr<Item>> items2;
	for (int i = 0; i < items.size(); i++) {
		auto objet2 = next(items1.begin(), i);
		items2.push_front(move(*objet2));
	}
	cout << "Liste 2" << endl;
	afficherListeItems(items2);

	//Question 1.3
	forward_list <unique_ptr<Item>> items3;
	auto objet3 = next(items1.begin(), 0);
	items2.push_front(move(*objet3));

	auto fl_index3 = items1.begin();
	advance(fl_index, 0);

	for (int i = 1; i < items.size(); i++) {
		auto objet3 = next(items1.begin(), i);
		items3.insert_after(fl_index3, move(*objet3));
		advance(fl_index, i);
	}
	cout << "Liste 3" << endl;
	afficherListeItems(items3);

	//Question 1.4
	vector <unique_ptr<Item>> items4;
	for (int i = 0; i < items.size(); i++) {
		auto objet4 = next(items1.begin(), i);
		items4.push_back(make_unique<Item>(**objet4));
	}
	cout << "Liste 4" << endl;
	afficherListeItems(items4);

	//Question 1.5
	auto premierElementListe = next(items1.begin(), 0);
	Film* premierFilm = dynamic_cast<Film*>((*premierElementListe).get());

	for (auto&& acteur : premierFilm->acteurs.enSpan()) {
		cout << acteur->nom << endl;
	}
	
	
}
