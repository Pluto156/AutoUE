from typing import List, Optional
from core.BaseLLMNode import BaseLLMNode  # Assumes BaseLLMNode is located in the same directory
import threading


class BaseLLMGraph:
    """
    Manages an LLM graph composed of BaseLLMNode nodes.

    Supports chain-style node addition:
    graph.AddNode(node1).AddNode(node2)...
    """

    def __init__(self, model=None):
        self.model = model
        # Ensure a real list is initialized
        self.nodes: List["BaseLLMNode"] = []
        # Optional thread lock to avoid race conditions in concurrent scenarios
        self._nodes_lock = threading.Lock()
        # Optional node count, kept in sync with nodes (useful for debugging)
        self.count = 0
        print(f"[DEBUG] Created BaseLLMGraph instance, current model = {model}")

    def reset_nodes(self):
        for node in self.nodes:
            node.reset()

    def AddNode(self, node: "BaseLLMNode") -> "BaseLLMGraph":
        """
        Add a BaseLLMNode to the graph and automatically set previous/next links.

        Can be called in a chain:
        graph.AddNode(A).AddNode(B).AddNode(C)

        This is a more robust (thread-safe) version that guards against
        empty lists and race conditions.
        """
        if not isinstance(node, BaseLLMNode):
            raise TypeError("AddNode() only accepts objects of type BaseLLMNode.")

        # Uniformly set the model
        if self.model is not None:
            node.set_model(self.model)

        # === Lock to avoid race conditions in concurrent scenarios ===
        with self._nodes_lock:
            # Defensive check: ensure nodes is a list
            if not isinstance(self.nodes, list):
                print("[WARN] self.nodes is not a list, resetting to []")
                self.nodes = []

            # Explicit length check (more explicit than `if not self.nodes`)
            if len(self.nodes) == 0:
                # First node logic
                node.isHead = True
                print(f"[DEBUG] Node {node.name} marked as head node (isHead=True)")
            else:
                # In concurrent environments, the list length may change
                # after the check, so guard access to the last element
                try:
                    prev = self.nodes[-1]
                except IndexError:
                    # Extremely rare race condition: list became empty
                    node.isHead = True
                    print(
                        f"[DEBUG][race] Failed to read last node, "
                        f"node {node.name} marked as head node (isHead=True)"
                    )
                else:
                    # Normal linking
                    prev.set_next_node(node)
                    node.set_prev_node(prev)

            # Append node and maintain count
            self.nodes.append(node)
            self.count = len(self.nodes)

        print(f"[DEBUG] Node added: {node.name}, current node count: {self.count}")
        return self

    # =====================================================
    # Get the first node (graph entry point)
    # =====================================================
    def get_first_node(self) -> Optional[BaseLLMNode]:
        return self.nodes[0] if self.nodes else None

    # =====================================================
    # Get the last node (graph exit point)
    # =====================================================
    def get_last_node(self) -> Optional[BaseLLMNode]:
        return self.nodes[-1] if self.nodes else None

    # =====================================================
    # Set the LLM model for all nodes in the graph
    # =====================================================
    def set_model(self, model):
        self.model = model
        for node in self.nodes:
            node.set_model(model)
        print(f"[DEBUG] LLM model set for all nodes: {model}")
        return self

    # =====================================================
    # Print the graph structure
    # =====================================================
    def print_graph(self):
        print("\n" + "=" * 60)
        print("[DEBUG] Current LLM Graph structure:")
        print("=" * 60)
        for i, node in enumerate(self.nodes):
            prev_name = node.prev_node.name if node.prev_node else "None"
            next_name = node.next_node.name if node.next_node else "None"
            print(f"[{i}] {node.name} | prev: {prev_name} | next: {next_name}")
        print("=" * 60 + "\n")
