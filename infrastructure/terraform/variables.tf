variable "project_id" {
  description = "The project ID to deploy to"
  type        = string
}

variable "region" {
  description = "The region to deploy to"
  type        = string
  default     = "europe-west2"
}

variable "cluster_name" {
  description = "The name of the GKE cluster"
  type        = string
  default     = "aarnn-cluster"
}

variable "node_count" {
  description = "The number of nodes in the GKE cluster"
  type        = number
  default     = 3
}
