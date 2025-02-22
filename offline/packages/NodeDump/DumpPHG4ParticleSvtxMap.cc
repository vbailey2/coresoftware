#include "DumpPHG4ParticleSvtxMap.h"

#include <phool/PHIODataNode.h>

#include <trackbase_historic/PHG4ParticleSvtxMap.h>

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>

typedef PHIODataNode<PHG4ParticleSvtxMap> MyNode_t;

DumpPHG4ParticleSvtxMap::DumpPHG4ParticleSvtxMap(const std::string &NodeName)
  : DumpObject(NodeName)
{
  return;
}

int DumpPHG4ParticleSvtxMap::process_Node(PHNode *myNode)
{
  PHG4ParticleSvtxMap *phg4particlesvtxmap = nullptr;
  MyNode_t *thisNode = static_cast<MyNode_t *>(myNode);
  if (thisNode)
  {
    phg4particlesvtxmap = thisNode->getData();
  }
  if (phg4particlesvtxmap)
  {
    *fout << "size " << phg4particlesvtxmap->size() << std::endl;
    for (auto iter = phg4particlesvtxmap->begin(); iter != phg4particlesvtxmap->end(); ++iter)
    {
      *fout << "Cluster: " << std::hex << iter->first << std::dec << std::endl;

      for (auto iter2 = (iter->second).begin(); iter2 != (iter->second).end(); ++iter2)
      {
        *fout << "weight: " << iter2->first << std::endl;
        for (auto iter3 = (iter2->second).begin(); iter3 != (iter2->second).end(); ++iter3)
        {
          *fout << "track id " << *iter3 << std::endl;
        }
      }
    }
  }
  return 0;
}
