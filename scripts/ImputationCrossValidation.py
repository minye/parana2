"""
Usage: ImputationCrossValidation.py PDIR ENET DTREE (cdf|pdf) [--relative] -o FILE

Arguments:
   PDIR         Directory containing the cross validation results
   ENET         Extant Network
   DTREE        Duplication history
Options:
   -h --help    show this
   --relative   compute relative (rather than absolute) ranks
   -o FILE      specify the output file [default: plot.pdf]
"""
from docopt import docopt

import glob
import itertools
from collections import defaultdict

import networkx as nx
import ete2
import numpy as np
import matplotlib.pyplot as plt
from progressbar import ProgressBar

def writeAdjList(G, fname):
	with open(fname,'wb') as ofile:
		ofile.write('# Multiline adjacency list with orphan nodes')
		for u in G.nodes():
			nstring = ' '.join( G.neighbors(u) )
			ofile.write('{0} {1}\n'.format(u, nstring))

class Edge(object):
	def __init__(self, u, v, p):
		self._u, self._v = sorted((u,v))
		self._p = p

	def __eq__(self, other):
		if isinstance(other, self.__class__):
			return (self._u == other._u) and (self._v == other._v)
		else:
			False

	def __ne__(self, other): return (not self.__eq__(other))

	def __str__(self):
		return "({0},{1}):{2}".format(self._u, self._v, self._p)

def pairsWithSelf(a):
    return list(itertools.combinations(a,2)) + [ (x,x) for x in a ]

def computeRank( tfile, tedge, allPossibleEdges, enet, randomize=False ):
	enet.remove_edge(tedge[0],tedge[1])
	missingEdges = set([ (u,v) for u,v in allPossibleEdges if not enet.has_edge(u,v) ])

	import random
	edgesFromFile = []
	with open(tfile,'rb') as ifile:
		for l in ifile:
			u,v,f,p = l.strip().split()
			p = float(p)
			if (u,v) in missingEdges or (v,u) in missingEdges: #(u in enet) and (v in enet) and (not enet.has_edge(u,v)):
				if randomize: p = random.uniform(0.0,1.0)
				edgesFromFile.append(Edge(u,v,p))
				missingEdges.discard((u,v))
				missingEdges.discard((v,u))

	assert(len(missingEdges) == 0)
	edges = list(enumerate(sorted(edgesFromFile, key = lambda x: x._p, reverse=True)))
	targetEdge = Edge(tedge[0],tedge[1],0.0)

	rank,edge = None,None
	for r,e in edges:
		if e == targetEdge:
			rank,edge = r,e
			break

	if rank is None:
		raise 'Hell!'

	relativeRank = rank / float(len(edgesFromFile)-1)
	enet.add_edge(tedge[0],tedge[1])

	return relativeRank

def plotHistogram( ranks, fname="" ):
		plt.hist(ranks, bins=50)
		plt.xlabel('Relative Rank of Imputed Edge')
		plt.ylabel('# of Imputed Edges with Given Relative Rank', labelpad=25)
		plt.grid(axis='both')
		plt.show()

def plotCDF( ranks, relative, fname="" ):
    num_bins = len(ranks)
    counts, bin_edges = np.histogram(ranks, bins=num_bins, normed=False)
    cdf = np.cumsum(counts, axis=0)
    bin_edges = list(bin_edges) #+ [1.0]
    cdf = list(cdf) #+ [cdf[-1]]
    
    # relative
    if relative:
        tot = np.max(cdf)
        cdf = [ c / float(tot) for c in cdf ]
        plt.plot(bin_edges[1:] + [1.0], cdf + [cdf[-1]] )
        plt.xlabel('Relative Rank of Imputed Edge')
        plt.ylabel('Fraction of Edges With Relative Rank < x')
        plt.grid(axis='both')
        plt.xlim((0.0,1.0))
        plt.ylim((0.0,1.0))
        plt.show()

def main():
	arguments = docopt(__doc__, version='ImputationCrossValidation v1.0')
	relative = arguments['--relative']

	extantNetwork = nx.read_adjlist(arguments['ENET'])

	pxml = ete2.Phyloxml()
	pxml.build_from_file( arguments['DTREE'] )
	p = pxml.phylogeny[0]
	
	leaves = filter( lambda x : x.find('LOST') == -1, pxml.phylogeny[0].get_leaf_names() )
	allPossibleEdges = filter( lambda (x,y): x.split('_')[-1] == y.split('_')[-1], pairsWithSelf(leaves) )
	print(len(allPossibleEdges))
	relativeRanks = []
	speciesRanks = defaultdict(list)

	with open(arguments['-o'],'wb') as ofile:
		progress = ProgressBar()
		for fname in progress(glob.glob("{0}/*@txt".format(arguments['PDIR']))):
			tstring = fname.split('@')[-2].split("#")
			tedge = (tstring[0], tstring[1])
			species = tstring[0].split('_')[-1]

			rr = computeRank( fname, tedge, allPossibleEdges, extantNetwork, randomize=False )
			ofile.write('{0}\t{1}\t{2}\n'.format(tedge[0],tedge[1],rr))
			relativeRanks.append(rr)
			speciesRanks[species].append(rr)

	relativeRanks = np.array(relativeRanks)

	for k,v in speciesRanks.iteritems():
		print("MEAN Relative Rank for Species {0}: is {1}".format(k, np.mean(v)))
		print("MEDIAN Relative Rank for Species {0}: is {1}".format(k, np.median(v)))

	params = { 'font.size' : 20,
	'lines.linewidth' : 3.0,
	}

	plt.rcParams.update(params)

	if ( arguments['pdf'] ):
		plotHistogram(relativeRanks)
	elif( arguments['cdf'] ):
		plotCDF(relativeRanks, relative=relative)

	print("MEAN RELATIVE RANK = {0}".format(relativeRanks.mean()))
	print("MEDIAN RELATIVE RANK = {0}".format(np.median(relativeRanks)))

if __name__ == "__main__":
	main()