#!/usr/bin/python

import sys
import re # For regex

import ryu_ofctl
from ryu_ofctl import *

def main(macHostA, macHostB):
    print "Installing flows for %s <==> %s" % (macHostA, macHostB)

    ##### FEEL FREE TO MODIFY ANYTHING HERE #####
    try:
        pathA2B = dijkstras(macHostA, macHostB)
        installPathFlows(macHostA, macHostB, pathA2B)
    except:
        raise


    return 0

# Installs end-to-end bi-directional flows in all switches
def installPathFlows(macHostA, macHostB, pathA2B):
    ##### YOUR CODE HERE #####
    
    for switch in pathA2B:
        dpid = str(switch['dpid'])
#        deleteAllFlows(dpid)
        print "delete all flows"
    
    for switch in pathA2B:
        flow = FlowEntry()
        flow2 = FlowEntry()

        flow2.in_port = switch['out_port']
        output2 = OutputAction(switch['in_port'])
       
        flow.in_port = switch['in_port']
        output = OutputAction(switch['out_port'])
        dpid = str(switch['dpid'])

        flow.addAction(output)
        flow2.addAction(output2)

        insertFlow(dpid,flow2)
        insertFlow(dpid,flow)
    
    print "flow finished"
    
    return

# Returns List of neighbouring DPIDs

def findNeighbours(dpid):
    
    print dpid

    if type(dpid) not in (int, long) or dpid < 0:
       raise TypeError("DPID should be a positive integer value")

    neighbours = []

    dpid = '000000000000000' +  str(dpid)
    ##### YOUR CODE HERE #####
    neighbour_links = listSwitchLinks(dpid)

    for link in neighbour_links['links']:
        
        if link['endpoint1']['dpid'] == dpid:
            neighbours.append(int(link['endpoint2']['dpid']))
    #    elif link['endpoint2']['dpid'] is not dpid and link['endpoint2']['dpid'] in neighbours"
    #        neighbours.append(link['endpoint2'])
    print neighbours
    return neighbours

# Calculates least distance path between A and B
# Returns detailed path (switch ID, input port, output port)
#   - Suggested data format is a List of Dictionaries
#       e.g.    [   {'dpid': 3, 'in_port': 1, 'out_port': 3},
#                   {'dpid': 2, 'in_port': 1, 'out_port': 2},
#                   {'dpid': 4, 'in_port': 3, 'out_port': 1},
#               ]
# Raises exception if either ingress or egress ports for the MACs can't be found
def dijkstras(macHostA, macHostB):

    # Optional helper function if you use suggested return format
    def nodeDict(dpid, in_port, out_port):
        assert type(dpid) in (int, long)
        assert type(in_port) is int
        assert type(out_port) is int
        return {'dpid': dpid, 'in_port': in_port, 'out_port': out_port}

    # Optional variables and data structures
    INFINITY = float('inf')
    distanceFromA = {} # Key = node, value = distance
    leastDistNeighbour = {} # Key = node, value = neighbour node with least distance from A
    pathAtoB = [] # Holds path information
    visitlist = []
    currentList = []
    ##### YOUR CODE HERE #####

    HostA_edge = getMacIngressPort(macHostA)
    HostB_edge = getMacIngressPort(macHostB)
    

    src_switch = int(HostA_edge['dpid'])
    dest_switch = int(HostB_edge['dpid'])

    cur_switch = src_switch
    currentList.append(src_switch)

    if src_switch == dest_switch:
        # when the two host are on the same switch
        pathAtoB.append(nodeDict(int(src_switch), int(HostA_edge['port']), int(HostB_edge['port'])))
        print "pathAtoB aaaaa = %s" % pathAtoB
        return pathAtoB

    distanceFromA[cur_switch] = 0

    while dest_switch not in currentList:
        temp = []
        for i in currentList:
           
            if i in visitlist:
                continue

            visitlist.append(i)
            neighbours = findNeighbours(i) # neighbours' DPIDs
            
            temp = temp + neighbours
          
            for j in neighbours:
                if j not in leastDistNeighbour:
                    leastDistNeighbour[j] = i
                    distanceFromA[j] = distanceFromA[i] + 1

                else:
                    if distanceFromA[j] > distanceFromA[i] + 1:
                        distanceFromA[j] = distanceFromA[i] + 1
                        leastDistNeighbour[j] = i

        currentList = temp
        
    
    path = [dest_switch]
    cur_switch = dest_switch
    while cur_switch is not src_switch:
        path.append(leastDistNeighbour[cur_switch])
        cur_switch = leastDistNeighbour[cur_switch]
    
    path.append(src_switch)
    path.reverse()
    
    
    # find pathAtoB
    # find the port from host A
    prev_Port = HostA_edge['port']
    
    switchLinkList = listLinks()['links']
    for i in range(len(path)-1):
        node_from = path[i]
        node_to = path[i+1]
        for link in switchLinkList:
            if int(link['endpoint1']['dpid']) == node_from and int(link['endpoint2']['dpid']) == node_to:
#                print node_from
                pathAtoB.append(nodeDict(int(node_from), int(prev_Port), int(link['endpoint1']['port'])))
                prev_Port = link['endpoint2']['port']
    pathAtoB.append(nodeDict(int(path[-1]), int(prev_Port), int(HostB_edge['port'])))

                
        


    # Some debugging output
    #print "leastDistNeighbour = %s" % leastDistNeighbour
    #print "distanceFromA = %s" % distanceFromA
    print "pathAtoB = %s" % pathAtoB

    return pathAtoB



# Validates the MAC address format and returns a lowercase version of it
def validateMAC(mac):
    invalidMAC = re.findall('[^0-9a-f:]', mac.lower()) or len(mac) != 17
    if invalidMAC:
        raise ValueError("MAC address %s has an invalid format" % mac)

    return mac.lower()

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print "This script installs bi-directional flows between two hosts"
        print "Expected usage: install_path.py <hostA's MAC> <hostB's MAC>"
    else:
        macHostA = validateMAC(sys.argv[1])
        macHostB = validateMAC(sys.argv[2])

        sys.exit( main(macHostA, macHostB) )
