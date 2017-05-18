#include "structures_molecule.hpp"
#include <string>
#include <vector>
#include <list>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

using namespace std;
using namespace boost::numeric::ublas;


//Implementations of class Atom functions

Atom::Atom(float x, float y, float z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

/*
 @return the x coordinate
 */
float Atom::getX()
{
    return x;
}

/*
 @return the y coordinate
 */
float Atom::getY()
{
    return y;
}

/*
 @return the z coordinate
 */
float Atom::getZ()
{
    return z;
}

/*
 return a string that describe the atom
 */
string Atom::toString()
{
    return "x = " + to_string(x) + " y = " + to_string(y) + " z = " + to_string(z);
}

/*
 The coordinates of the atom are modified in function of the matrix given as parameter
 @param transformationMatrix the matrix that describes the trasformation
 */
void Atom::transform(matrix<float>  transformationMatrix)
{
    //transform the original vector in omogeneous coordinates in order to do the transformation
    matrix<float> homogeneusCoordinatesPoint(4, 1);
    homogeneusCoordinatesPoint(0, 0) = x;
    homogeneusCoordinatesPoint(1, 0) = y;
    homogeneusCoordinatesPoint(2, 0) = z;
    homogeneusCoordinatesPoint(3, 0) = 1;
    
    homogeneusCoordinatesPoint = prod(transformationMatrix, homogeneusCoordinatesPoint);
    
    this->x = homogeneusCoordinatesPoint(0, 0) / homogeneusCoordinatesPoint(3, 0);
    this->y = homogeneusCoordinatesPoint(1, 0) / homogeneusCoordinatesPoint(3, 0);
    this->z = homogeneusCoordinatesPoint(2, 0) / homogeneusCoordinatesPoint(3, 0);
}



//Implementations of class Molecule functions

int Molecule::getAtomIndex(Atom atom)
{
    for (int i = 0; i<atoms.size(); i++)
    {
        Atom a = atoms.at(i);
        
        if (a.getX() == atom.getX() && a.getY() == atom.getY() && a.getZ() == atom.getZ())
            return i;
    }
    return 0;
}

/*
 @rotator the rotator that we want to verify if it is in a cycle
 @return a boolean that indicates if the rotator is in a cycle
 */
bool Molecule::Molecule::isRotatorInCycle(std::pair<Atom, Atom> rotator)
{
    int first = getAtomIndex(rotator.first);
    int second = getAtomIndex(rotator.second);
    
    std::vector<int> atomsToBeConsidered;
    std::vector<int> successorsIndex;
    
    for (int a : getSuccessor(second))
        if (a != first && a != second)
            atomsToBeConsidered.push_back(a);
    
    while (!atomsToBeConsidered.empty())
    {
        int nextAtom = atomsToBeConsidered.back();
        atomsToBeConsidered.pop_back();
        if (std::find(successorsIndex.begin(), successorsIndex.end(), nextAtom) == successorsIndex.end() && nextAtom != second)
        {
            successorsIndex.push_back(nextAtom);
            for (int a : getSuccessor(nextAtom))
                atomsToBeConsidered.push_back(a);
        }
    }
    for (int successor : successorsIndex)
        if (successor == first)
            return true;
    return false;
}

Molecule::Molecule(string name)
{
    this->name = name;
    atoms.clear();
    links.clear();
}

void Molecule::setName(string name)
{
    this->name = name;
}

string Molecule::getName()
{
    return name;
}

/*
 it adds an atom to the molecule
 @atom the atom that has to be added
 */
void Molecule::addAtom(Atom atom)
{
    atoms.push_back(atom);
    list<int> link;
    links.push_back(link);
}

/*
 it adds an edge to the molecule
 @src the first element of the edge
 @dest the second element of the edge
 */
void Molecule::setEdge(int src, int dest)
{
    links.at(src).push_back(dest);
    links.at(dest).push_back(src);
}

/*
 It returns the list of rotators in the molecule and so all the non-terminal and not-in-cycle links
 @return the list of rotator in the molecule
 */
std::vector<pair<Atom, Atom>> Molecule::getRotators()
{
    std::vector<pair<Atom, Atom>> rotatorsWithCycles;
    std::vector<pair<Atom, Atom>> rotators;
    std::vector<Atom>::iterator firstAtomIt = atoms.begin();
    std::vector<list<int> >::iterator firstListElement = links.begin();
    
    //cycle that identifies all non-terminal links
    for (list<int> link : links)
    {
        for (list<int>::iterator it = link.begin(); it != link.end(); ++it)
        {
            std::vector<Atom>::iterator secondAtomIt = atoms.begin();
            
            bool isRotator = (link.size() > 1) && ((*(firstListElement + *it)).size() > 1);
            if (isRotator)/*link non finali e non in un ciclo*/
            {
                secondAtomIt += *it;
                pair<Atom, Atom> rotator = make_pair(*firstAtomIt, *secondAtomIt);
                rotatorsWithCycles.push_back(rotator);
            }
        }
        firstAtomIt += 1;
    }
    
    //cycles that identifies all rotator deleting those that are in a cycle
    for (pair<Atom, Atom> rotator : rotatorsWithCycles)
    {
        if (!isRotatorInCycle(rotator))
            rotators.push_back(rotator);
    }
    
    return rotators;
}

/*
 @param atom the atom of which we want to know the successor
 @return the list of atoms connected to the atom passed as a parameter
 */
std::vector<int> Molecule::getSuccessor(int atom)
{
    std::vector<int> successors;
    list<int> link = links.at(atom);
    
    for (list<int>::iterator it = link.begin(); it != link.end(); ++it)
        successors.push_back(*it);
    
    return successors;
}

/*
@rotator the rotator of which we want to know the successors
@return the successors of the rotator
*/
std::vector<int> getRotatorSuccessors(std::pair<Atom, Atom> rotator)
{
	int first = getAtomIndex(rotator.first);
	int second = getAtomIndex(rotator.second);

	std::vector<int> atomsToBeConsidered;
	std::vector<int> successorsIndex;

	for (int a : getSuccessor(second))
		atomsToBeConsidered.push_back(a);

	while (!atomsToBeConsidered.empty())
	{
		int nextAtom = atomsToBeConsidered.back();
		atomsToBeConsidered.pop_back();
		if (std::find(successorsIndex.begin(), successorsIndex.end(), nextAtom) == successorsIndex.end() &&
			nextAtom != first && nextAtom != second)
		{
			successorsIndex.push_back(nextAtom);
			for (int a : getSuccessor(nextAtom))
				atomsToBeConsidered.push_back(a);
		}
	}
	return successorsIndex;
}

std::vector<Atom> getAtoms()
{
	return atoms;
}

std::vector<std::list<int>> getLinks()
{
	return links;
}

void transform(matrix<float>  transformationMatrix, int index)
{
	atoms.at(index).transform(transformationMatrix);
}

/*
 @return a string that describes the molecule
 */
string Molecule::toString()
{
    //creates a string with the list of atoms
    string molecule = "The molecule has the following atoms";
    for (Atom atom : atoms)
    {
        molecule += "\n";
        molecule += atom.toString();
    }
    
    //creates a string with the structure of the graph
    molecule += "\n\nGraph structure:";
    for (int atom = 0; atom < links.size(); atom++)
    {
        std::vector<int> successors = getSuccessor(atom);
        molecule += "\nAtom ";
        molecule += to_string(atom);
        molecule += " linked to: ";
        for (int succ : successors)
        {
            molecule += to_string(succ);
            molecule += ", ";
        }
    }
    return molecule;
}
