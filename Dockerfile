# Use an official C++ runtime as a parent image
FROM ubuntu:latest

# Set the working directory in the container to /app

WORKDIR /app

ARG POSTGRES_USER
ARG POSTGRES_PASSWORD

ENV POSTGRES_USER=${POSTGRES_USER}
ENV POSTGRES_PASSWORD=${POSTGRES_PASSWORD}
ENV POSTGRES_DB=neurons

# Copy the compiled executable from your host to the current location in your docker image
COPY ./build/AARNN /app/AARNN
COPY ./build/db_connection.conf /app/db_connection.conf 
COPY ./build/simulation.conf /app/simulation.conf 

# Make port 80 available to the world outside this container (optional)
EXPOSE 80

# Install necessary libraries for running the application
RUN apt-get update
RUN apt-get install -y libpqxx-dev

# Run the application when the container launches
CMD ["./AARNN"]
