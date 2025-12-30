#pragma once
#include <vector>
#include <algorithm>
#include <optional>
#include <cassert>

namespace BixEngine
{
    template<typename T, typename Allocator = std::allocator<T>>
    class Array : public std::vector<T, Allocator>
    {
    public:
        
        using Base = std::vector<T, Allocator>;
        using size_type =  Base::size_type;
        using value_type =  Base::value_type;
        using reference =  Base::reference;
        using const_reference =  Base::const_reference;
        using iterator =  Base::iterator;
        using const_iterator =  Base::const_iterator;

        using Base::vector;
        
        /**
         * @brief Ajoute un élément à la fin (Alias de push_back plus lisible).
         */
        void Add(const T& item)
        {
            this->push_back(item);
        }
        
        void Add(T&& item)
        {
            this->push_back(std::move(item));
        }

        /**
         * @brief Ajoute un élément uniquement s'il n'est pas déjà présent.
         * @return true si ajouté, false s'il existait déjà.
         */
        bool AddUnique(const T& item)
        {
            if (Contains(item))
                return false;
            
            this->push_back(item);
            return true;
        }

        /**
         * @brief Vérifie si l'élément existe dans le tableau.
         */
        [[nodiscard]] bool Contains(const T& item) const
        {
            return std::find(this->begin(), this->end(), item) != this->end();
        }

        /**
         * @brief Trouve l'index d'un élément.
         * @return L'index ou -1 si pas trouvé.
         */
        bool Find(const T& item, size_type& outIndex) const
        {
            auto it = std::find(this->begin(), this->end(), item);
            if (it != this->end())
            {
                outIndex = std::distance(this->begin(), it);
                return true;
            }
            return false;
        }

        /**
         * @brief Supprime la première occurrence de l'élément.
         * @return true si un élément a été supprimé.
         */
        bool Remove(const T& item)
        {
            auto it = std::find(this->begin(), this->end(), item);
            if (it != this->end())
            {
                this->erase(it);
                return true;
            }
            
            return false;
        }

        /**
         * @brief Supprime l'élément à l'index donné.
         */
        void RemoveAt(size_type index)
        {
            assert(index < this->size() && "Index hors limites dans Array::RemoveAt");
            this->erase(this->begin() + index);
        }

        /**
         * Supprime l'élément en le remplaçant par le dernier du tableau, puis réduit la taille.
         * @warning Ne préserve PAS l'ordre des éléments !
         */
        void RemoveSwap(size_type index)
        {
            assert(index < this->size() && "Index hors limites dans Array::RemoveSwap");
            
            if (index < this->size() - 1)
            {
                (*this)[index] = std::move(this->back());
            }
            
            this->pop_back();
        }
        
        /**
         * @brief Version RemoveSwap qui cherche l'élément par valeur.
         */
        bool RemoveSwap(const T& item)
        {
            auto it = std::find(this->begin(), this->end(), item);
            if (it != this->end())
            {
                size_type index = std::distance(this->begin(), it);
                RemoveSwap(index);
                return true;
            }
            
            return false;
        }

        /**
         * @brief Vide le tableau mais garde la capacité mémoire (évite les réallocations futures).
         */
        void Reset()
        {
            this->clear();
        }
        
        /**
         * @brief Renvoie le dernier élément (Safe version avec check optionnel).
         */
        [[nodiscard]] T& Last() 
        {
            assert(!this->empty());
            return this->back();
        }

        [[nodiscard]] const T& Last() const
        {
            assert(!this->empty());
            return this->back();
        }
    };
    
    template<typename T>
    using TArray = Array<T>;
}