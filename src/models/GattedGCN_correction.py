import torch
import torch.nn as nn
import torch.nn.functional as F
import dgl
import dgl.function as fn
import sys
from functools import partial
import os.path as osp
import time
import numpy as np

"""
    Graph Transformer
    
"""
from src.layers.graph_transformer_layer import GraphTransformerLayer
from src.layers.mlp_readout_layer import MLPReadout


class GraphTransformerNet(nn.Module):
    def __init__(self, dev):
        super().__init__()

        in_dim_node = 9  # node_dim (feat is an integer)
        self.clust_space_norm = "twonorm"
        hidden_dim = 80  # before 80
        out_dim = 80
        n_classes = 4
        num_heads = 8
        in_feat_dropout = 0.0
        dropout = 0.0
        n_layers = 10
        self.n_layers = n_layers
        self.layer_norm = False
        self.batch_norm = False
        self.residual = True
        self.dropout = dropout
        self.n_classes = n_classes
        self.device = dev
        self.lap_pos_enc = False
        self.wl_pos_enc = False
        max_wl_role_index = 100
        self.readout = "sum"
        self.output_dim = n_classes

        self.embedding_h = nn.Linear(in_dim_node, hidden_dim)  # node feat is an integer
        self.embedding_h.weight.data.copy_(torch.eye(hidden_dim, in_dim_node))
        self.in_feat_dropout = nn.Dropout(in_feat_dropout)

        self.layers = nn.ModuleList(
            [
                GraphTransformerLayer(
                    hidden_dim,
                    hidden_dim,
                    num_heads,
                    dropout,
                    self.layer_norm,
                    self.batch_norm,
                    self.residual,
                )
                for _ in range(n_layers - 1)
            ]
        )
        self.layers.append(
            GraphTransformerLayer(
                hidden_dim,
                out_dim,
                num_heads,
                dropout,
                self.layer_norm,
                self.batch_norm,
                self.residual,
            )
        )
        self.MLP_layer = MLPReadout(out_dim, 1)

    def forward(self, g_batch):
        g = g_batch

        ############################## Embeddings #############################################
        h = g.ndata["h"]
        # input embedding
        h = self.embedding_h(h)
        h = self.in_feat_dropout(h)

        # GraphTransformer Layers
        for conv in self.layers:
            h = conv(g, h)

        hg = dgl.sum_nodes(g, "h")

        output = self.MLP_layer(hg)
        return output
