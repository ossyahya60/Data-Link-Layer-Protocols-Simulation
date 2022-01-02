package section1;

@license(LGPL);
//
// TODO documentation
//
network Network
{
    double delay;
    int timeout;
    int windowSize;
    
    @display("bgb=619,254");
    submodules:
        coordinator: Coordinator {
            @display("p=170,123;is=l");
        }
        node0: Node {
            @display("p=49,32");
        }
        node1: Node {
            @display("p=266,32");
        }
        node2: Node {
            @display("p=460,131");
        }
        node3: Node {
            @display("p=540,39");
        }
        node4: Node {
            @display("p=65.965004,207.08");
        }
        node5: Node {
            @display("p=281.395,207.08");
        }
    connections:
        node4.out --> node5.in[0];
        node5.out --> node4.in[0];
        node2.out --> node3.in[0];
        node3.out --> node2.in[0];
        node0.out --> node1.in[0];
        node1.out --> node0.in[0];
        coordinator.out[0] --> node0.in[1];
        coordinator.out[1] --> node1.in[1];
        coordinator.out[2] --> node2.in[1];
        coordinator.out[3] --> node3.in[1];
        coordinator.out[4] --> node4.in[1];
        coordinator.out[5] --> node5.in[1];
}