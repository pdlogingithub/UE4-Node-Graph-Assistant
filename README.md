# UE4-Node-Graph-Assistant User Guide

Support page: https://forums.unrealengine.com/unreal-engine/marketplace/1435240-node-graph-assistant  
Marketplace page: https://www.unrealengine.com/marketplace/node-graph-assistant  
中文说明：https://github.com/pdlogingithub/UE4-Node-Graph-Assistant/blob/master/Doc/%E8%99%9A%E5%B9%BB4%E8%93%9D%E5%9B%BE%E5%8A%A9%E6%89%8B%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.md  

1. Right click multi-connect: right click on pin to multi-connect,right click on panel to cancel,you can also pan the graph when dragging a wire.  
![1](Resource/1.4/drag_pan_multi-connect.gif)

2. Left click multi-connect: Click on pin to start free panning,zooming and multi-connecting.  
![2](Resource/1.4/click_pan_multi-connect.gif)

3. Shift click multi-connect: When dragging a wire, hold down shift to start free panning,zooming and multi-connecting.  
![3](Resource/1.4/shift_pan_multi-connect.gif)

4. Dupli-wire: Shift click or drag on pin to duplicate wire.  
![4](Resource/1.4/dupli_wire.gif)

5. Rearrange nodes: Press <kbd>Alt</kbd>+<kbd>R</kbd> to rearrange nodes, most suitable for small block of nodes.   
![5](Resource/1.4/rearrange.gif)

6. Bypass node: Press <kbd>Alt</kbd>+<kbd>X</kbd> will remove selected nodes on wire.  
![6](Resource/1.4/bypass.gif)

7. Create node on wire: Right click on wire to create node on wire.  
![7](Resource/1.4/insert.gif)

8. Highlight wire: Left click to highlight wire, hold down shift to highlight multiple wires, middle mouse double click on pin to highlight all connected wires.  
![8](Resource/1.4/highlight.gif)

9. Cutoff wires: Hold <kbd>Alt</kbd> and drag across wires to break them.  
![9](Resource/1.4/cutoff.gif)

10. Select linked nodes: Middle mouse double click on a node to select all connected nodes.  
Depending on whether the left or right area of the node is clicked on, this will select all children or all parents.  
Or use the hotkey <kbd>Alt</kbd>+<kbd>A</kbd>, or <kbd>Alt</kbd>+<kbd>D</kbd>.  
![10](Resource/1.4/select_linked.gif)

11. Lazy connect: When dragging off of a pin, the wire will snap to the closest pin on the hovered node.  
(Hold <kbd>Shift</kbd> and dragging over pins to batch connect.it may appears laggy if "live preview" button is on for material graph.)  
![11](Resource/1.5/lazy_connect.gif)
 
12. Auto connect: When moving a node, its pins will align to surrounding connectible pins. Releasing mouse will commit connections.  
Holding <kbd>Alt</kbd> will suppress auto connect.   
![12](Resource/1.5/auto_connect.gif)

13. Wire style: press the toolbar button to cycle through wire styles.  
![13](Resource/1.5/wire_style.gif)

14. Shake node off wire: Shake a node quickly while dragging to bypass this node from wires.  
![14](Resource/1.6/shake_node_off_wire.gif)

15. Duplicate with inputs: Duplicate selected nodes and keep input wires with <kbd>Alt</kbd>+<kbd>V</kbd>  
![15](Resource/1.6/dupli_node_with_input.gif)

16. Insert node on wire:  
![16](Resource/1.6/insert_node_on_wire.gif)

17.  Exchange wires: Exchange two sets of wires with <kbd>Alt</kbd>+<kbd>T</kbd>  
(cursor position will direct which wires to operate on if there are multiple plausible wire sets.)  
![17](Resource/1.6/exchange_wires.gif)

Go to the editor preferences to see more settings.

![14](Resource/1.5/instruction_plugin.png)  
![15](Resource/1.5/instruction_keybind.png)  
![16](Resource/1.5/instruction_config.png)  
