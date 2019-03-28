import ryu_ofctl


dpid = '1'
ryu_ofctl.deleteAllFlows(dpid)


flow = ryu_ofctl.FlowEntry()
flow2 = ryu_ofctl.FlowEntry()

output_2 = ryu_ofctl.OutputAction(2)
output_3 = ryu_ofctl.OutputAction(3)
output_1 = ryu_ofctl.OutputAction(1)

flow.in_port = 1
flow2.in_port = 3

flow.addAction(output_2)
flow.addAction(output_3)
flow2.addAction(output_2)
flow2.addAction(output_1)


ryu_ofctl.insertFlow(dpid, flow)
ryu_ofctl.insertFlow(dpid, flow2)
