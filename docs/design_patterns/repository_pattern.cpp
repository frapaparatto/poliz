/*
 * Repository Design Pattern in C++
 * article: 
 *  https://www.geeksforgeeks.org/system-design/repository-design-pattern/
 *
 * Abstraction layer
 * business ------ data
 *
 * what do you want? Provide like a valid 'API' to access data in an consistent
 * way. You don't worry about implementation, you only care about working with
 * data
 *
 * business ------ repository ------ data
 *               (you don't know
 *               implementation details,
 *               you have a set of tools
 *               to interact with data
 *               that are always valid)
 *
 * Example of a librarian in a library: you don't search the book by yourself,
 * you ask the librarian to help you. You don' have to worrry about where the
 * book is stored
 *
 * Example of implmentation
 * ------------------------
 * Problem statement
 * You are developing an e-commerce app that needs to manage products. The app
 * should be able to add, retrieve, update and delete products.
 * The idea is simple: instead of interacting directly with db, use the repo
 * pattern to handle those operations
 *
 */

#include <string>
#include <vector>

class Product {
 public:
  int id;
  std::string name;
  float price;

  Product(int id, std::string name, float price)
      : id(id), name(std::move(name)), price(price) {}
};

// Interface for the repository
class ProductRepository {
 public:
  virtual void addProduct(const Product& product) = 0;
  virtual Product getProductById(int productId) = 0;
  virtual void updateProduct(const Product& product) = 0;
  virtual void deleteProduct(int productId) = 0;
};

// Concrete implementation of the repository (in-memory repository)
class InMemoryProductRepository : public ProductRepository {
private:
    std::vector<Product> products;

public:
    void addProduct(const Product& product) override {
        products.push_back(product);
    }

    Product getProductById(int productId) override {
        for (const auto& product : products) {
            if (product.id == productId) {
                return product;
            }
        }
        return Product(-1, "Not Found", 0.0); // Return a default product if not found
    }

    void updateProduct(const Product& updatedProduct) override {
        for (auto& product : products) {
            if (product.id == updatedProduct.id) {
                product = updatedProduct;
                return;
            }
        }
    }

    void deleteProduct(int productId) override {
        products.erase(std::remove_if(products.begin(), products.end(),
            [productId](const Product& product) { return product.id == productId; }),
            products.end());
    }
};



