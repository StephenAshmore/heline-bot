#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <string>

#include "hlt.hpp"
#include "networking.hpp"
// #include "log.h"

using namespace std;

ofstream out;

int threshold = 20;
const float STILL_MODIFIER = 7.5;


// cluster struct:
struct cluster {
  unsigned int id;
  unsigned int strength;
};

struct location {
  unsigned short int x;
  unsigned short int y;
};

// void floydWarshall(unsigned int ** dist, unsigned int ** next, const unsigned int size);
void clustering(hlt::GameMap& map, vector<cluster> & clusters);
int wrap(int l, int size);
void resetClusters(unsigned int** clustering, unsigned int width, unsigned int height);

int main() {
  // Initialization:
  out.open("log.txt", std::ios_base::out);
  srand(time(NULL));
  out << "LOG FILE:" << endl;

  std::cout.sync_with_stdio(0);

  unsigned char myID;
  hlt::GameMap map;
  getInit(myID, map);
  sendInit("heline-bot");
  // end init.

  // All Pairs Shortest Paths Distances Matrix:
  unsigned int size = map.width * map.height;
  // unsigned int** distances;
  // distances = new unsigned int*[size];
  // for ( unsigned int i = 0; i < size; i++ ) {
  //   distances[i] = new unsigned int[size];
  // }

  std::set<hlt::Move> final_moves;

  // log(tempStr.c_str());
  //Clustering map:
  unsigned int** clustering = new unsigned int*[map.height];
  for ( unsigned int i = 0; i < map.height; i++ ) {
    clustering[i] = new unsigned int[map.width];
  }

  // Cluster list:
  vector<cluster> clusters;
  std::queue<location> m_queue;

  bool moved = false;

  location neighbor_right;
  location neighbor_left;
  location neighbor_above;
  location neighbor_below;

  while(true) {
    std::queue<location> empty;
    std::swap(m_queue,empty);
    unsigned int clusterID = 0;
    resetClusters(clustering, map.width, map.height);
    final_moves.clear();

    getFrame(map);

    // Find clusters:
    // clustering(map, clusters);
    bool done = false;
    bool stop = false;
    int count = 0;
    unsigned short initY = 0; unsigned short initX = 0;
    while ( !done ) {
      stop = false;
      // find un-clustered cell:
      // remember where we stopped
      for ( unsigned short y = 0; y < map.height && !stop; y++ ) {
        for ( unsigned short x = 0; x < map.width; x++ ) {
          if ( clustering[y][x] == 0 ) {
            clusterID++;
            // out << "CLUSTER ID: " << clusterID << endl;
            // add this one to queue.
            location newLoc;
            newLoc.x = x;
            newLoc.y = y;
            initX = x; initY = y;
            m_queue.push(newLoc);
            clustering[y][x] = clusterID;
            count++;
            stop = true;
            break;
          }
        }
      }

      if ( !stop ) {
        done = true;
        break;
      }

      // go through queue:
      location& cur = m_queue.front();
      hlt::Site& site = map.getSite({cur.x,cur.y});
      unsigned int curStr = site.strength;

      // out << "Current Strength is: " << curStr << endl;

      // add to clusters:
      cluster newCluster;
      newCluster.id = clusterID;
      newCluster.strength = curStr;
      clusters.push_back(newCluster);

      // Add neighbors of first originator to m_queue.
      // dequee this guy.
      neighbor_right.x = wrap(cur.x + 1, map.width);   neighbor_right.y = wrap(cur.y, map.height);
      neighbor_left.x = wrap(cur.x - 1, map.width);    neighbor_left.y = wrap(cur.y, map.height);
      neighbor_above.x = wrap(cur.x, map.width);       neighbor_above.y = wrap(cur.y - 1, map.height);
      neighbor_below.x = wrap(cur.x, map.width);       neighbor_below.y = wrap(cur.y + 1, map.height);
      m_queue.push(neighbor_right); m_queue.push(neighbor_left);
      m_queue.push(neighbor_above); m_queue.push(neighbor_below);
      m_queue.pop();

      while ( !m_queue.empty()) {
        location& cur = m_queue.front();

        hlt::Site& site = map.getSite({cur.x, cur.y});
        if ( clustering[cur.y][cur.x] == 0 &&
          (site.strength <= curStr + threshold && site.strength >= curStr - threshold ) )
        {
          // out << "Adding " << cur.x << ", " << cur.y << " to the cluster." << endl;
          // if so, add it to the clustering matrix
          // and add neighbors to queue:
          clustering[cur.y][cur.x] = clusterID;
          neighbor_right.x = wrap(cur.x + 1, map.width);   neighbor_right.y = wrap(cur.y, map.height);
          neighbor_left.x = wrap(cur.x - 1, map.width);    neighbor_left.y = wrap(cur.y, map.height);
          neighbor_above.x = wrap(cur.x, map.width);       neighbor_above.y = wrap(cur.y - 1, map.height);
          neighbor_below.x = wrap(cur.x, map.width);       neighbor_below.y = wrap(cur.y + 1, map.height);
          m_queue.push(neighbor_right); m_queue.push(neighbor_left);
          m_queue.push(neighbor_above); m_queue.push(neighbor_below);
          count++;
        }
        m_queue.pop();
      }
    }


    for(unsigned short a = 0; a < map.height; a++) {
      for(unsigned short b = 0; b < map.width; b++) {
        hlt::Site& site = map.getSite({ b, a });
        if ( site.owner == myID ) {
          if ( !moved && site.strength < site.production * STILL_MODIFIER) {
            moves.insert({ { b, a }, STILL });
          }
          else if ( !moved ) {
            moves.insert({ { b, a }, (unsigned char)(rand() % 5) });
          }
        }
      }
    }

    sendFrame(final_moves);
  }

  out.close();
  return 0;
}

void resetClusters(unsigned int** clustering, unsigned int width, unsigned int height) {
  for ( unsigned short y = 0; y < height; y++ ) {
    for ( unsigned short x = 0; x < width; x++ ) {
      clustering[y][x] = 0;
    }
  }
}

int wrap(int l, int size) {
  size = size - 1;
  int ans;
  if ( l < 0 )
    ans = size - (size % (l * -1)); // modulus here
  else if ( l >= size )
    ans = ((size+1) % l);
  else
    ans = l;

  // out << "Wrapping: " << l << " to " << ans << endl;
  return ans;
}

// void clustering(hlt::GameMap& map, vector<cluster> & clusters) {
//   bool inCluster = false;
//   for ( unsigned short y = 0; y < map.height; y++ ) {
//     for ( unsigned short x = 0; x < map.width; x++ ) {
//       hlt::Site& site = map.getSite({x,y});
//       if ( )
//     }
//   }
// }
// always a square graph
// void floydWarshall(unsigned int ** dist, unsigned int ** map, const unsigned int size) {
//   // The Wikipedia article is horrible. Besides mis-representing (in my opinion) the canonical form of the Floyd–Warshall algorithm, it presents a buggy pseudocode.
//   // It’s much easier (and more direct) not to iterate over indices but over vertices. Furthermore, each predecessor (usually denoted π, not next), needs to point to its, well, predecessor, not the current temporary vertex.
//   // With that in mind, we can fix their broken pseudocode.
//
//   for ( unsigned int i = 0; i < size; i++ ){
//     for ( unsigned int j = 0; j < size; j++ ) {
//       dist[i][j] = 0;
//     }
//   }
//
//   // for each vertex i
//   //     for each vertex j
//   //         if w(u,v) = ∞ then
//   //             next[i][j] ← NIL
//   //         else
//   //             next[i][j] ← i
//
//
//                 // for each edge (u,v)
//                     // dist[u][v] ← w(u,v)  // the weight of the edge (u,v)
//
//   for ( unsigned int i = 0; i < size; i++ ) {
//     for ( unsigned int j = 0; j < size; j++ ) {
//       if ( ) {
//
//       }
//     }
//   }
//
//
//   for ( unsigned int k = 0; k < size; k++ ) {
//     for ( unsigned int i = 0; i < size; i++ ) {
//       for ( unsigned int j = 0; j < size; j++ ) {
//         if ( dist[i][k] + dist[k][j] < dist[i][j]) {
//           dist[i][j] = dist[i][k] + dist[k][j];
//           next[i][j] = next[k][j];
//         }
//       }
//     }
//   }
//
// }
