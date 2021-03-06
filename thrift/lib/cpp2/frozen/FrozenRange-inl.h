/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

namespace apache {
namespace thrift {
namespace frozen {

/**
 * Layout specialization for range types, excluding those covered by 'string'
 * type. Frozen arrays support random-access and iteration without thawing.
 */
template <class T, class Item>
struct ArrayLayout : public LayoutBase {
  typedef ArrayLayout LayoutSelf;

  Field<size_t> distance;
  Field<size_t> count;
  Field<Item> item;

  ArrayLayout() : LayoutBase(typeid(T)), distance(1), count(2), item(3) {}

  FieldPosition layout(LayoutRoot& root, const T& coll, LayoutPosition self) {
    size_t n = coll.size();
    size_t itemBytes = item.layout.size;
    size_t itemBits = itemBytes ? 0 : item.layout.bits;
    size_t dist = root.layoutBytesDistance(
        self.start, itemBits ? (n * itemBits + 7) / 8 : n * itemBytes);

    FieldPosition pos = startFieldPosition();
    pos = root.layoutField(self, pos, distance, dist);
    pos = root.layoutField(self, pos, count, n);

    LayoutPosition write{self.start + dist, 0};
    FieldPosition writeStep(itemBytes, itemBits);
    return layoutItems(root, coll, self, pos, write, writeStep);
  }

  virtual FieldPosition layoutItems(LayoutRoot& root,
                                    const T& coll,
                                    LayoutPosition self,
                                    FieldPosition pos,
                                    LayoutPosition write,
                                    FieldPosition writeStep) {
    FieldPosition noField; // not really used
    for (auto& it : coll) {
      root.layoutField(write, noField, this->item, it);
      write = write(writeStep);
    }

    return pos;
  }

  void freeze(FreezeRoot& root, const T& coll, FreezePosition self) const {
    size_t n = coll.size();
    size_t itemBytes = item.layout.size;
    size_t itemBits = itemBytes ? 0 : item.layout.bits;
    folly::MutableByteRange range;
    size_t dist;
    root.appendBytes(self.start,
                     itemBits ? (n * itemBits + 7) / 8 : n * itemBytes,
                     range,
                     dist);

    root.freezeField(self, distance, dist);
    root.freezeField(self, count, n);

    FreezePosition write{self.start + dist, 0};
    FieldPosition writeStep(itemBytes, itemBits);
    freezeItems(root, coll, self, write, writeStep);
  }

  virtual void freezeItems(FreezeRoot& root,
                           const T& coll,
                           FreezePosition self,
                           FreezePosition write,
                           FieldPosition writeStep) const {
    for (const auto& it : coll) {
      root.freezeField(write, item, it);
      write = write(writeStep);
    }
  }

  void thaw(ViewPosition self, T& out) const {
    out.clear();
    auto outIt = std::back_inserter(out);
    auto v = view(self);
    for (auto it = v.begin(); it != v.end(); ++it) {
      *outIt++ = it.thaw();
    }
  }

  void print(std::ostream& os, int level) const override {
    LayoutBase::print(os, level);
    os << "range of " << folly::demangle(type.name());
    distance.print(os, "distance", level + 1);
    count.print(os, "count", level + 1);
    item.print(os, "items", level + 1);
  }

  void clear() override {
    LayoutBase::clear();
    distance.clear();
    count.clear();
    item.clear();
  }

  void save(schema::Schema& schema, schema::Layout& layout) const override {
    LayoutBase::save(schema, layout);
    distance.save(schema, layout);
    count.save(schema, layout);
    item.save(schema, layout);
  }

  void load(const schema::Schema& schema,
            const schema::Layout& layout) override {
    LayoutBase::load(schema, layout);
    distance.load(schema, layout);
    count.load(schema, layout);
    item.load(schema, layout);
  }

  /**
   * A view of a range, which produces views of items upon indexing or iterator
   * dereference
   */
  class View : public ViewBase<View, ArrayLayout, T> {
    typedef typename Layout<Item>::View ItemView;
    class Iterator;

    static ViewPosition indexPosition(const byte* start,
                                      size_t i,
                                      const LayoutBase* itemLayout) {
      if (!itemLayout) {
        return {start, 0};
      } else if (itemLayout->size) {
        return ViewPosition{start + itemLayout->size * i, 0};
      } else {
        return ViewPosition{start, itemLayout->bits * i};
      }
    }

   public:
    typedef ItemView value_type;
    typedef ItemView reference_type;
    typedef Iterator iterator;
    typedef Iterator const_iterator;

    View() : data_(nullptr), count_(0), itemLayout_(nullptr) {}
    View(const LayoutSelf* layout, ViewPosition self)
        : ViewBase<View, ArrayLayout, T>(layout, self),
          data_(nullptr),
          count_(0),
          itemLayout_(&layout->item.layout) {
      size_t dist;
      thawField(self, layout->count, count_);
      if (count_) {
        thawField(self, layout->distance, dist);
        data_ = self.start + dist;
      }
    }

    ItemView operator[](ptrdiff_t index) const {
      return itemLayout_->view(indexPosition(data_, index, itemLayout_));
    }

    const_iterator begin() const {
      return const_iterator(data_, 0, itemLayout_);
    }

    const_iterator end() const {
      return const_iterator(data_, count_, itemLayout_);
    }

    bool empty() const { return !count_; }
    size_t size() const { return count_; }

   private:
    /**
     * Simple iterator on a range, with additional '.thaw()' member for thawing
     * a single member.
     */
    class Iterator : public std::iterator<std::random_access_iterator_tag,
                                          ItemView,
                                          std::ptrdiff_t,
                                          void,
                                          void> {
     public:
      Iterator(const byte* data, size_t index, const Layout<Item>* itemLayout)
          : index_(index),
            data_(data),
            itemLayout_(itemLayout) {}

      ViewPosition position() const {
        return indexPosition(data_, index_, itemLayout_);
      }

      ItemView operator*() const { return itemLayout_->view(position()); }
      ItemView operator->() const { return operator*(); }

      Item thaw() const {
        Item item;
        itemLayout_->thaw(position(), item);
        return item;
      }

      ptrdiff_t operator-(const Iterator& other) {
        return index_ - other.index_;
      }

      Iterator& operator++() {
        ++index_;
        return *this;
      }
      Iterator& operator+=(ptrdiff_t delta) {
        index_ += delta;
        return *this;
      }
      Iterator& operator--() {
        --index_;
        return *this;
      }
      Iterator& operator-=(ptrdiff_t delta) {
        index_ -= delta;
        return *this;
      }
      Iterator operator++(int) const {
        Iterator ret(*this);
        ++ret;
        return ret;
      }
      Iterator operator--(int) const {
        Iterator ret(*this);
        --ret;
        return ret;
      }
      Iterator operator+(ptrdiff_t delta) const {
        Iterator ret(*this);
        ret += delta;
        return ret;
      }
      Iterator operator-(ptrdiff_t delta) const {
        Iterator ret(*this);
        ret -= delta;
        return ret;
      }

      bool operator==(const Iterator& other) const {
        return index_ == other.index_ && data_ == other.data_;
      }

      bool operator!=(const Iterator& other) const { return !(*this == other); }

     private:
      size_t index_;
      const byte* data_;
      const Layout<Item>* itemLayout_;
    };

    const byte* data_;
    size_t count_;
    const Layout<Item>* itemLayout_;
  };

  View view(ViewPosition self) const { return View(this, self); }
};

template <class T, class A>
struct Layout<std::vector<T, A>, void> : public ArrayLayout<std::vector<T, A>,
                                                            T> {};
}
}
}
