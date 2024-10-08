# =================================================================================
# Title       : Makefile
# Description : Makefile for managing Terraform commands on GCP
# Author      : Paul B. Isaac's
# Created     : [25-JUL-2024]
# License     : All rights reserved. Any use of open-source or third-party solutions will be clearly identified.
# =================================================================================

SHELL := /bin/bash

.PHONY: all help activate_testing clean get plan show show_current format apply build_environments rebuild_environment destroy

# Default config file
CONFIG_FILE ?= environments.cfg

# GCP project ID
GCP_PROJECT_ID ?= "your-gcp-project-id"

all: build_environments

help:
	@echo "Usage: make [target] [CONFIG_FILE=config_file] [ENV=environment] [INSTANCE=instance_number] [GCP_PROJECT_ID=project_id]"
	@echo "Available targets:"
	@echo " * help                - Show this help message"
	@echo " * show                - List what the plan will apply"
	@echo " * clean               - Delete the executed plan, so no files are left behind"
	@echo " * get                 - Update the terraform modules"
	@echo " * format              - Execute terraform fmt"
	@echo " * activate_testing    - Activate testing workspace"
	@echo " * plan                - Create infrastructure plan"
	@echo " * apply               - Apply infrastructure plan"
	@echo " * show_current        - Show current state"
	@echo " * build_environments  - Build all specified environments and instances"
	@echo " * rebuild_environment - Rebuild a specific environment and instance"
	@echo " * destroy             - Destroy all resources in the specified environment and instance"

activate_testing:
	@echo "Activated testing workspace"
	@terraform workspace select testing

clean:
	@echo "Cleaning up"
	@rm -rf tmp

get:
	@echo "Updating modules"
	@terraform get -update

plan: format get
	@echo "Checking Infrastructure"
	@mkdir -p tmp
	@terraform -chdir=./infrastructure/terraform plan -var="environment=$(ENV)" -var="instance_number=$(INSTANCE)" -var="project_id=$(GCP_PROJECT_ID)" -parallelism=80 -refresh=true -out ../../tmp/plan_$(ENV)_$(INSTANCE)

show:
	@echo "Showing plan to apply"
	@terraform show ./tmp/plan_$(ENV)_$(INSTANCE)

show_current:
	@echo "Showing current state"
	@terraform show

format:
	@echo "Formatting existing code"
	@terraform -chdir=./infrastructure/terraform fmt -recursive

apply:
	@echo "Applying changes to Infrastructure"
	@terraform -chdir=./infrastructure/terraform apply -parallelism=80 ../../tmp/plan_$(ENV)_$(INSTANCE)
	@echo "Cleaning up after apply"
	$(MAKE) clean

build_environments:
	@echo "Building environments and instances from $(CONFIG_FILE)"
	@grep -v '^\s*#' $(CONFIG_FILE) | while IFS=, read -r ENV INSTANCES; do \
		if [ "$$INSTANCES" -gt 0 ]; then \
			for i in `seq 1 $$INSTANCES`; do \
				echo "Building $$ENV-$$i"; \
				$(MAKE) plan ENV=$$ENV INSTANCE=$$i GCP_PROJECT_ID=$(GCP_PROJECT_ID); \
				$(MAKE) apply ENV=$$ENV INSTANCE=$$i GCP_PROJECT_ID=$(GCP_PROJECT_ID); \
			done; \
		else \
			echo "Skipping $$ENV as it has 0 instances"; \
		fi; \
	done

rebuild_environment:
	@if [ -z "$(ENV)" ] || [ -z "$(INSTANCE)" ]; then \
		echo "Error: ENV and INSTANCE variables must be set"; \
		exit 1; \
	fi
	@echo "Rebuilding environment $(ENV)-$(INSTANCE)"
	$(MAKE) plan ENV=$(ENV) INSTANCE=$(INSTANCE) GCP_PROJECT_ID=$(GCP_PROJECT_ID)
	$(MAKE) apply ENV=$(ENV) INSTANCE=$(INSTANCE) GCP_PROJECT_ID=$(GCP_PROJECT_ID)

destroy:
	@if [ -z "$(ENV)" ] || [ -z "$(INSTANCE)" ]; then \
		echo "Error: ENV and INSTANCE variables must be set"; \
		exit 1; \
	fi
	@echo "Destroying all resources in environment $(ENV)-$(INSTANCE)"
	@terraform -chdir=./infrastructure/terraform destroy -var="environment=$(ENV)" -var="instance_number=$(INSTANCE)" -var="project_id=$(GCP_PROJECT_ID)" -parallelism=80
