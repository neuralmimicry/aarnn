# Use an official C++ runtime as a parent image
FROM ubuntu:latest

# Set the working directory in the container to /app
WORKDIR /app

# Copy the compiled executable from your host to the current location in your docker image
COPY ./build/AARNN /app/AARNN

#Database configuration connection infromation
COPY ./build/db_connection.conf /app/db_connection.conf 

#Simulation data
COPY ./build/simulation.conf /app/simulation.conf 

# Install necessary libraries for running the application
RUN apt-get update
RUN apt-get install -y libpqxx-dev

# Make port 80 available to the world outside this container (optional)
EXPOSE 80


# Run the application when the container launches
CMD ["./AARNN"]
