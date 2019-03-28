import ryu_ofctl
flow1 = ryu_ofctl.FlowEntry()
flow2 = ryu_ofctl.FlowEntry()
flow3 = ryu_ofctl.FlowEntry()

output_1 = ryu_ofctl.OutputAction(1)
output_2 = ryu_ofctl.OutputAction(2)
output_3 = ryu_ofctl.OutputAction(3)


flow1.in_port = 1
flow3.in_port = 3

flow1.addAction(output_2)
flow3.addAction(output_2)

flow2.addAction(output_1)
flow2.addAction(output_3)

dpid = '1'

ryu_ofctl.insertFlow(dpid, flow1)
ryu_ofctl.insertFlow(dpid, flow3)
#ryu_ofctl.insertFlow(dpid, flow2)
