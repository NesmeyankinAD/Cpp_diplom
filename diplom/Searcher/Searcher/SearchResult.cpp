#include "SearchResult.h"

#include <ostream>

std::ostream& operator<<(std::ostream& os, const SearchResult& sr)
{
    os  << "URL: " << sr.url
        << ", Score: " << sr.score
        << ", DocumentID: " << sr.document_id;
    return os;
}