#include "DumpSvtxPHG4ParticleMap.h"

#include <phool/PHIODataNode.h>

#include <trackbase_historic/SvtxPHG4ParticleMap.h>

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>

typedef PHIODataNode<SvtxPHG4ParticleMap> MyNode_t;

DumpSvtxPHG4ParticleMap::DumpSvtxPHG4ParticleMap(const std::string &NodeName)
  : DumpObject(NodeName)
{
  return;
}

int DumpSvtxPHG4ParticleMap::process_Node(PHNode *myNode)
{
  SvtxPHG4ParticleMap *svtxphg4particlemap = nullptr;
  MyNode_t *thisNode = static_cast<MyNode_t *>(myNode);
  if (thisNode)
  {
    svtxphg4particlemap = thisNode->getData();
  }
  if (svtxphg4particlemap)
  {
    *fout << "size " << svtxphg4particlemap->size() << std::endl;
    for (auto iter = svtxphg4particlemap->begin(); iter != svtxphg4particlemap->end(); ++iter)
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
